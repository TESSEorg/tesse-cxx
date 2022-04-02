#pragma once

#include <ttg.h>
// needed for madness::hashT and xterm_debug
#include <madness/world/world.h>
#include "pmw.h"
#include "lapack.hh"

#include <ttg.h>
// needed for madness::hashT and xterm_debug
#include <madness/world/world.h>
#include "pmw.h"

namespace lauum {

/* FLOP macros taken from DPLASMA */
double FMULS_DLAUUM(double __n) { return 0.0; }
double FADDS_DLAUUM(double __n) { return 0.0; }
double FLOPS_DLAUUM(double __n) { return      FMULS_DLAUUM(__n) +       FADDS_DLAUUM(__n); }

static int event_lauum_syrk_startkey, event_lauum_syrk_endkey;
static int event_lauum_gemm_startkey, event_lauum_gemm_endkey;
static int event_lauum_trmm_startkey, event_lauum_trmm_endkey;
static int event_lauum_lauum_startkey, event_lauum_lauum_endkey;
#define EVENT_LAUUM_INFO_CONVERTER "I{int}"

/**
 * Wrapper around parsec_profiling_trace_flags to enable/disable at will
 */
int lauum_parsec_profiling_trace_flags(parsec_profiling_stream_t* context, int key,
                                       uint64_t event_id, uint32_t taskpool_id,
                                       const void *info, uint16_t flags )
{
  int rc = 0;
#if USE_PARSEC_PROF_API
  if (profiling_enabled) {
    init_prof_thread();
    rc = parsec_profiling_trace_flags(context, key, event_id, taskpool_id, info, flags);
  }
#endif // USE_PARSEC_PROF_API
  return rc;
}

template <typename T>
auto make_lauum(const MatrixT<T>& A,
                ttg::Edge<Key1, MatrixTile<T>>& input_disp, // from the dispatcher
                ttg::Edge<Key2, MatrixTile<T>>& to_syrk_C,
                ttg::Edge<Key2, MatrixTile<T>>& output_result)
{
  auto f = [=](const Key1& key,
               MatrixTile<T>&& tile_kk,
               std::tuple<ttg::Out<Key2, MatrixTile<T>>, ttg::Out<Key2, MatrixTile<T>>>& out){
    const int K = key.K;

    std::cout << "LAUUM " << key << std::endl;
    lauum_parsec_profiling_trace_flags(prof, event_lauum_lauum_startkey, K, PROFILE_OBJECT_ID_NULL,
                                       &key, PARSEC_PROFILING_EVENT_HAS_INFO);

    
    lapack::lauum(lapack::Uplo::Lower, tile_kk.rows(), tile_kk.data(), tile_kk.rows());
    
    lauum_parsec_profiling_trace_flags(prof, event_lauum_lauum_endkey, K, PROFILE_OBJECT_ID_NULL, NULL, 0);

    /* send the tile to output */
    if(K < A.rows()-1) {
        std::cout << "LAUUM(" << key << ") sending to SYRK(" << Key2{K+1, K} << ")" << std::endl;
        ttg::send<0>(Key2{K+1, K}, std::move(tile_kk), out);
    } else if (K == A.rows()-1) {
        ttg::send<1>(Key2{K, K}, std::move(tile_kk), out);
    }
  };
  return ttg::make_tt(f, ttg::edges(input_disp), ttg::edges(to_syrk_C, output_result), "LAUUM", 
                      {"tile_kk"}, {"to_syrk_C", "output_result"});
}

template <typename T>
auto make_trmm(const MatrixT<T>& A,
               ttg::Edge<Key2, MatrixTile<T>>& input_kk, // will come from the dispatcher
               ttg::Edge<Key2, MatrixTile<T>>& input_kn,
               ttg::Edge<Key3, MatrixTile<T>>& to_gemm_C,
               ttg::Edge<Key2, MatrixTile<T>>& output_result)
{
  auto f = [=](const Key2& key,
               const MatrixTile<T>& tile_kk,
               MatrixTile<T>&& tile_kn,
               std::tuple<ttg::Out<Key3, MatrixTile<T>>, ttg::Out<Key2, MatrixTile<T>>>& out){
    const int K = key.I;
    const int N = key.J;

    auto mb = tile_kn.rows();
    assert(tile_kk.rows() == mb);

    std::cout << "TRMM " << key << std::endl;
    lauum_parsec_profiling_trace_flags(prof, event_lauum_trmm_startkey, K, PROFILE_OBJECT_ID_NULL, NULL, 0);

    blas::trmm(blas::Layout::ColMajor,
               blas::Side::Left,
               lapack::Uplo::Lower,
               blas::Op::Trans,
               blas::Diag::NonUnit,
               mb, mb, 1.0,
               tile_kk.data(), mb,
               tile_kn.data(), mb);

    lauum_parsec_profiling_trace_flags(prof, event_lauum_trmm_endkey, K, PROFILE_OBJECT_ID_NULL, NULL, 0);

    if(K < A.rows()-1) {
      std::cout << "TRMM(" << key << ") sending to C of GEMM(" << Key3{K+1, N, K} << ")" << std::endl;
      ttg::send<0>(Key3{K+1, N, K}, std::move(tile_kn), out);
    } else if(K == A.rows()-1) {
      /* send the tile to output */
      ttg::send<1>(Key2{K, N}, std::move(tile_kn), out);
    }
  };
  return ttg::make_tt(f, ttg::edges(input_kk, input_kn), ttg::edges(to_gemm_C, output_result), "TRMM", 
                      {"tile_kk", "tile_kn"}, {"to_GEMM_C", "output_result"});
}


template <typename T>
auto make_syrk(const MatrixT<T>& A,
               ttg::Edge<Key2, MatrixTile<T>>& input_kn, // will from the dispatcher
               ttg::Edge<Key2, MatrixTile<T>>& input_nn, 
               ttg::Edge<Key2, MatrixTile<T>>& to_syrk_nn,
               ttg::Edge<Key2, MatrixTile<T>>& output_result)
{
  auto f = [=](const Key2& key,
               const MatrixTile<T>& tile_kn,
               MatrixTile<T>&& tile_nn,
               std::tuple<ttg::Out<Key2, MatrixTile<T>>, // syrk_nn
                          ttg::Out<Key2, MatrixTile<T>>  // result
                         >& out){
    const int K = key.I;
    const int N = key.J;

    auto mb = tile_kn.rows();
    assert(tile_nn.rows() == mb);

    std::cout << "SYRK " << key << std::endl;
    lauum_parsec_profiling_trace_flags(prof, event_lauum_trmm_startkey, K, PROFILE_OBJECT_ID_NULL, NULL, 0);

    blas::syrk(blas::Layout::ColMajor,
               lapack::Uplo::Lower,
               blas::Op::Trans,
               mb, mb, 
               1.0, tile_kn.data(), mb,
               1.0, tile_nn.data(), mb);

    lauum_parsec_profiling_trace_flags(prof, event_lauum_trmm_endkey, K, PROFILE_OBJECT_ID_NULL, NULL, 0);

    if(K < A.rows()-1) {
        std::cout << "SYRK(" << key << ") sending to nn of SYRK(" << Key2{K+1, N} << ")" << std::endl;
        ttg::send<0>(Key2{K+1, N}, tile_kn, out);
    } else if(K == A.rows()-1) {
        ttg::send<1>(Key2{N, N}, tile_kn, out);
    }
  };
  return ttg::make_tt(f, ttg::edges(input_kn, input_nn), 
                         ttg::edges(to_syrk_nn, output_result), "SYRK", 
                      {"tile_kn", "tile_nn"}, {"SYRK_nn", "output_result"});
}

template <typename T>
auto make_gemm(const MatrixT<T>& A,
               ttg::Edge<Key3, MatrixTile<T>>& input_A,
               ttg::Edge<Key3, MatrixTile<T>>& input_B,
               ttg::Edge<Key3, MatrixTile<T>>& input_C,
               ttg::Edge<Key3, MatrixTile<T>>& to_gemm_C,
               ttg::Edge<Key2, MatrixTile<T>>& output_result)
{
  auto f = [=](const Key3& key,
               const MatrixTile<T>& input_A,
               const MatrixTile<T>& input_B,
               MatrixTile<T>&& input_C,
               std::tuple<ttg::Out<Key3, MatrixTile<T>>, // gemm_C
                          ttg::Out<Key2, MatrixTile<T>> // output_result
                         >& out){
    const int K = key.I;
    const int N = key.J;
    const int M = key.K;

    auto mb = input_A.rows();
    assert(input_B.rows() == mb);
    assert(input_C.rows() == mb);

    std::cout << "GEMM " << key << std::endl;
    lauum_parsec_profiling_trace_flags(prof, event_lauum_gemm_startkey, K, PROFILE_OBJECT_ID_NULL, NULL, 0);

    blas::gemm(blas::Layout::ColMajor,
               blas::Op::Trans,
               blas::Op::NoTrans,
               mb, mb, mb, 
               1.0, input_A.data(), mb,
                    input_B.data(), mb, 
               1.0, input_C.data(), mb);

    lauum_parsec_profiling_trace_flags(prof, event_lauum_gemm_endkey, K, PROFILE_OBJECT_ID_NULL, NULL, 0);

    if(K < A.rows()-1) {
        std::cout << "GEMM(" << key << ") sending to C of GEMM(" << Key3{K+1, N, M} << ")" << std::endl;
        ttg::send<0>(Key3{K+1, N, M}, std::move(input_C), out);
    } else if(K == A.rows()-1) {
        ttg::send<1>(Key2{M, N}, std::move(input_C), out);
    }
  };
  return ttg::make_tt(f, ttg::edges(input_A, input_B, input_C), 
                         ttg::edges(to_gemm_C, output_result), "GEMM", 
                      {"A", "B", "C"}, {"GEMM_C", "output result"});
}


template<typename T>
auto make_dispatcher(const MatrixT<T>& A,
                     ttg::Edge<Key2, MatrixTile<T>>& input,
                     ttg::Edge<Key1, MatrixTile<T>>& to_lauum,
                     ttg::Edge<Key2, MatrixTile<T>>& to_syrk,
                     ttg::Edge<Key2, MatrixTile<T>>& to_trmm_A,
                     ttg::Edge<Key2, MatrixTile<T>>& to_trmm_B,
                     ttg::Edge<Key3, MatrixTile<T>>& to_gemm_A,
                     ttg::Edge<Key3, MatrixTile<T>>& to_gemm_B)
{
  auto f = [=](const Key2& key,
               MatrixTile<T>&&tile,
               std::tuple<ttg::Out<Key1, MatrixTile<T>>,
                          ttg::Out<Key2, MatrixTile<T>>,
                          ttg::Out<Key2, MatrixTile<T>>,
                          ttg::Out<Key2, MatrixTile<T>>,
                          ttg::Out<Key3, MatrixTile<T>>,
                          ttg::Out<Key3, MatrixTile<T>>>& out) {
    std::cout << "LAUUM_dispatch called with " << key << std::endl;
    std::vector<Key1> keylist_lauum;
    std::vector<Key2> keylist_syrk;
    std::vector<Key2> keylist_trmm_A;
    std::vector<Key2> keylist_trmm_B;
    std::vector<Key3> keylist_gemm_A;
    std::vector<Key3> keylist_gemm_B;

    if(key.I == key.J) {
        std::cout << "LAUUM_Dispatch(" << key << ") sending to LAUUM(" << Key1{key.I} << ")" << std::endl;
        keylist_lauum.reserve(1);
        keylist_lauum.push_back(Key1{key.I});
        if(key.I > 0) {
            keylist_trmm_A.reserve(key.J);
            for(int n = 0; n < key.J; n++) {
                std::cout << "LAUUM_Dispatch(" << key << ") sending to kn of TRMM(" << Key2{key.J, n} << ")" << std::endl;
                keylist_trmm_A.push_back(Key2{key.J, n});
            }
        }
    } 
    if(key.I > key.J) {
        keylist_syrk.reserve(1);
        std::cout << "LAUUM_Dispatch(" << key << ") sending to SYRK(" << key << ")" << std::endl;
        keylist_syrk.push_back(key);
        keylist_trmm_B.reserve(1);
        std::cout << "LAUUM_Dispatch(" << key << ") sending to nn of TRMM(" << key << ")" << std::endl;
        keylist_trmm_B.push_back(key);
        if(key.J > 0) {
            keylist_gemm_A.reserve(key.J);
            for(int n = 0; n < key.J; n++) {
                std::cout << "LAUUM_Dispatch(" << key << ") sending to A of GEMM(" << Key3{key.I, n, key.J} << ")" << std::endl;
                keylist_gemm_A.push_back(Key3{key.I, n, key.J});      
            }
        }
    }
    if(key.I > key.J + 1) {
        keylist_gemm_B.reserve(key.I - key.J + 1);
        for(int n = key.J+1; n < key.I; n++) {
            std::cout << "LAUUM_Dispatch(" << key << ") sending to B of GEMM(" << Key3{key.I, key.J, n} << ")" << std::endl;
            keylist_gemm_B.push_back(Key3{key.I, key.J, n});
        }
    }
    ttg::broadcast<0, 1, 2, 3, 4, 5>(std::make_tuple(keylist_lauum, keylist_syrk, keylist_trmm_A, keylist_trmm_B, keylist_gemm_A, keylist_gemm_B), std::move(tile), out);
  };

  return ttg::make_tt(f, ttg::edges(input), ttg::edges(to_lauum, to_syrk, to_trmm_A, to_trmm_B, to_gemm_A, to_gemm_B), 
                      "LAUUM Dispatch", {"Input"}, {"LAUUM", "SYRK", "TRMM_A", "TRMM_B", "GEMM_A", "GEMM_B"});
}

auto make_lauum_ttg(MatrixT<double> &A, ttg::Edge<Key2, MatrixTile<double>>&input, ttg::Edge<Key2, MatrixTile<double>>&output ) {
  auto keymap1 = [&](const Key1& key) {
    //std::cout << "Key " << key << " is at rank " << A.rank_of(key.K, key.K) << std::endl;
    return A.rank_of(key.K, key.K);
  };

  auto keymap2a = [&](const Key2& key) {
    //std::cout << "Key " << key << " is at rank " << A.rank_of(key.I, key.J) << std::endl;
    return A.rank_of(key.I, key.J);
  };
  auto keymap2b = [&](const Key2& key) {
    //std::cout << "Key " << key << " is at rank " << A.rank_of(key.J, key.J) << std::endl;
    return A.rank_of(key.J, key.J);
  };

  auto keymap3 = [&](const Key3& key) {
    //std::cout << "Key " << key << " is at rank " << A.rank_of(key.K, key.J) << std::endl;
    return A.rank_of(key.K, key.J);
  };

  ttg::Edge<Key1, MatrixTile<double>> disp_lauum("disp_lauum");

  ttg::Edge<Key2, MatrixTile<double>> disp_syrk("disp_syrk"),
                                      disp_trmm_A("disp_trmm_A"),
                                      disp_trmm_B("disp_trmm_B"),
                                      lauum_syrk_nn("lauum_syrk_nn"),
                                      syrk_syrk_nn("syrk_syrk_nn"),
                                      syrk_nn("syrk_nn");

  ttg::Edge<Key3, MatrixTile<double>> disp_gemm_A("disp_gemm_A"),
                                      disp_gemm_B("disp_gemm_B"),
                                      trmm_gemm_C("trmm_gemm_C"),
                                      gemm_gemm_C("gemm_gemm_C"),
                                      gemm_C("gemm_C");

  auto tt_dispatch = make_dispatcher(A, input, disp_lauum, disp_syrk, disp_trmm_A, disp_trmm_B, disp_gemm_A, disp_gemm_B);
  tt_dispatch->set_keymap(keymap2a);

  auto tt_lauum = make_lauum(A, disp_lauum, lauum_syrk_nn, output);
  tt_lauum->set_keymap(keymap1);

  syrk_nn = ttg::fuse(lauum_syrk_nn, syrk_syrk_nn);
  auto tt_syrk  = make_syrk(A, disp_syrk, syrk_nn, syrk_syrk_nn, output);
  tt_syrk->set_keymap(keymap2b);

  auto tt_trmm = make_trmm(A, disp_trmm_A, disp_trmm_B, trmm_gemm_C, output);
  tt_trmm->set_keymap(keymap2a);

  gemm_C = ttg::fuse(trmm_gemm_C, gemm_gemm_C);
  auto tt_gemm  = make_gemm(A, disp_gemm_A, disp_gemm_B, gemm_C, gemm_gemm_C,output);
  tt_gemm->set_keymap(keymap3);

  std::vector<std::unique_ptr<ttg::TTBase>> ops(5);
  ops[0] = std::move(tt_dispatch);
  ops[1] = std::move(tt_lauum);
  ops[2] = std::move(tt_syrk);
  ops[3] = std::move(tt_trmm);
  ops[4] = std::move(tt_gemm);
  auto ins = std::make_tuple(tt_dispatch->template in<0>());
  auto outs = std::make_tuple(tt_lauum->template out<1>());

  return make_ttg(std::move(ops), ins, outs, "LAUUM TTG");
}

}