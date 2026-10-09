//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) Pierre-Anthony Lemieux
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//***************************************************************************/
// This file is part of the OpenJPH software implementation.
// File: test_quant_weights.cpp
// Author: Pierre-Anthony Lemieux
// Date: 02 October 2026
//***************************************************************************/

#include <cmath>
#include <vector>

#include "ojph_mem.h"
#include "ojph_file.h"
#include "ojph_codestream.h"
#include "ojph_params.h"
#include "gtest/gtest.h"

///////////////////////////////////////////////////////////////////////////////
// Tests for param_qcd::set_irrev_quant(comp_idx, delta, weights, num_weights).
//
// The headers are generated in memory, and the QCD and QCC marker segments
// are parsed to check that the step sizes they carry are the ones expected
// from delta_b = delta_r / (sqrt(G_b) * sqrt(w_b)), with the expected values
// computed here independently of the library.

namespace {

  // squared norms of the 9/7 synthesis filters (Table 1 of the Qfactor
  // guideline, WG1N101553), for decomposition levels 1 to 3; the high-pass
  // filter is scaled by 2, as in the library, hence the factor of 4 below
  const double NORM2_L[3] = { 1.965908, 4.122410, 8.416737 };
  const double NORM2_H[3] = { 0.520218, 0.967216, 2.079257 };

  // square roots of the 9/7 synthesis energy gains, for up to 3 levels
  const float GAIN_L[4] = { 1.0f, (float)std::sqrt(NORM2_L[0]),
    (float)std::sqrt(NORM2_L[1]), (float)std::sqrt(NORM2_L[2]) };
  const float GAIN_H[3] = { (float)std::sqrt(4.0 * NORM2_H[0]),
    (float)std::sqrt(4.0 * NORM2_H[1]), (float)std::sqrt(4.0 * NORM2_H[2]) };

  // square roots of the nominal visual weights for 4:4:4 YCbCr content (Table
  // 2 of the Qfactor guideline), for 3 decomposition levels, in the order
  // {LH1, HL1, HH1, LH2, HL2, HH2, LH3, HL3, HH3, LL3}
  const float CB_SQRT_W[10] = { 0.0863f, 0.0863f, 0.0263f,
                                0.2564f, 0.2564f, 0.1362f,
                                0.4691f, 0.4691f, 0.3346f, 1.0f };
  const float CR_SQRT_W[10] = { 0.1835f, 0.1835f, 0.0773f,
                                0.4130f, 0.4130f, 0.2598f,
                                0.6464f, 0.6464f, 0.5040f, 1.0f };

  // the square roots of the visual weights, as expected by set_irrev_quant
  const std::vector<float> CB_W(CB_SQRT_W, CB_SQRT_W + 10);
  const std::vector<float> CR_W(CR_SQRT_W, CR_SQRT_W + 10);

  // decoded step sizes of a QCD/QCC, in codestream order: LL, then for each
  // level from the coarsest to the finest HL, LH, HH
  struct quant { bool found; ojph::ui8 style; std::vector<float> delta; };

  float decode_delta(ojph::ui16 v)
  {
    int exp = v >> 11, mant = v & 0x7FF;
    return std::ldexp(1.0f + (float)mant / 2048.0f, -exp);
  }

  ///////////////////////////////////////////////////////////////////////////
  // Writes the headers of a num_comps-component image, 9/7 wavelet, no color
  // transform. The callback configures the QCD/QCC.
  template<class F>
  void make_headers(ojph::mem_outfile& out, ojph::ui32 num_comps,
                    ojph::ui32 num_decomps, F configure,
                    bool reversible = false)
  {
    ojph::codestream cs;
    ojph::param_siz siz = cs.access_siz();
    siz.set_image_extent(ojph::point(64, 64));
    siz.set_num_components(num_comps);
    for (ojph::ui32 c = 0; c < num_comps; ++c)
      siz.set_component(c, ojph::point(1, 1), 8, false);
    siz.set_image_offset(ojph::point(0, 0));
    siz.set_tile_size(ojph::size(0, 0));
    siz.set_tile_offset(ojph::point(0, 0));

    ojph::param_cod cod = cs.access_cod();
    cod.set_num_decomposition(num_decomps);
    cod.set_color_transform(false);
    cod.set_reversible(reversible);

    configure(cs.access_qcd());

    out.open();
    cs.write_headers(&out);
  }

  ///////////////////////////////////////////////////////////////////////////
  // Finds the QCD (comp < 0) or the QCC of a component in the main header.
  quant find_quant(const ojph::mem_outfile& out, int comp,
                   ojph::ui32 num_decomps)
  {
    quant q = { false, 0, std::vector<float>() };
    const ojph::ui8* p = out.get_data();
    size_t len = (size_t)out.get_used_size();
    size_t pos = 2; // skip SOC
    while (pos + 4 <= len)
    {
      ojph::ui16 marker = (ojph::ui16)((p[pos] << 8) | p[pos + 1]);
      if (marker == 0xFF90) break; // SOT
      ojph::ui16 seg_len = (ojph::ui16)((p[pos + 2] << 8) | p[pos + 3]);
      size_t q0 = pos + 4;
      bool match = false;
      if (comp < 0 && marker == 0xFF5C)
        match = true;
      else if (comp >= 0 && marker == 0xFF5D && p[q0++] == comp) // 1-byte Cqcc
        match = true;
      if (match)
      {
        q.found = true;
        q.style = p[q0++];
        EXPECT_EQ(q.style & 0x1F, 2); // scalar expounded
        for (ojph::ui32 i = 0; i < 1 + 3 * num_decomps; ++i, q0 += 2)
          q.delta.push_back(
            decode_delta((ojph::ui16)((p[q0] << 8) | p[q0 + 1])));
        return q;
      }
      pos += (size_t)seg_len + 2;
    }
    return q;
  }

  ///////////////////////////////////////////////////////////////////////////
  // Expected step sizes in codestream order, given weights in the API order
  // {LH1, HL1, HH1, ..., LHN, HLN, HHN, LLN}
  std::vector<float> expected(float delta, const std::vector<float>& w,
                              ojph::ui32 n)
  {
    std::vector<float> e;
    e.push_back(delta / (GAIN_L[n] * GAIN_L[n] * w[3 * n]));
    for (ojph::ui32 d = n; d > 0; --d)
    {
      float gl = GAIN_L[d], gh = GAIN_H[d - 1];
      e.push_back(delta / (gh * gl * w[(d - 1) * 3 + 1])); // HL
      e.push_back(delta / (gl * gh * w[(d - 1) * 3 + 0])); // LH
      e.push_back(delta / (gh * gh * w[(d - 1) * 3 + 2])); // HH
    }
    return e;
  }

  void expect_close(const std::vector<float>& got,
                    const std::vector<float>& want)
  {
    ASSERT_EQ(got.size(), want.size());
    for (size_t i = 0; i < got.size(); ++i)
      // the step size is quantized to an 11-bit mantissa; the gain values
      // here are 5 digits
      EXPECT_NEAR(got[i] / want[i], 1.0f, 1.5e-3f) << "subband " << i;
  }

}

///////////////////////////////////////////////////////////////////////////////
// Weights on one component of two end up in that component's QCC, with every
// subband's step size equal to delta / (sqrt(G_b) * sqrt(w_b)). The weights are all
// different, so a subband mapped to the wrong weight is detected. The other
// component keeps using the QCD, with unit weights.
TEST(QuantWeights, ValuesInQcc)
{
  const ojph::ui32 n = 3;
  // all distinct, so a swapped subband is detected
  std::vector<float> w = CB_W;
  ojph::mem_outfile out;
  make_headers(out, 2, n, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(0.01f);
    qcd.set_irrev_quant(1, 0.02f, w.data(), w.size());
  });

  quant q1 = find_quant(out, 1, n);
  ASSERT_TRUE(q1.found);
  expect_close(q1.delta, expected(0.02f, w, n));

  // the other component keeps using the QCD, without weights
  EXPECT_FALSE(find_quant(out, 0, n).found);
  quant q0 = find_quant(out, -1, n);
  ASSERT_TRUE(q0.found);
  expect_close(q0.delta, expected(0.01f, std::vector<float>(10, 1.0f), n));
}

///////////////////////////////////////////////////////////////////////////////
// Weights set for the whole image end up in the QCD and apply to every
// component that has no QCC.
TEST(QuantWeights, ValuesInQcdForAllComponents)
{
  const ojph::ui32 n = 3;
  std::vector<float> w = CB_W;
  ojph::mem_outfile out;
  make_headers(out, 2, n, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(0.01f, w.data(), w.size());
  });

  quant q = find_quant(out, -1, n);
  ASSERT_TRUE(q.found);
  expect_close(q.delta, expected(0.01f, w, n));
  EXPECT_FALSE(find_quant(out, 0, n).found);
  EXPECT_FALSE(find_quant(out, 1, n).found);
}

///////////////////////////////////////////////////////////////////////////////
// Weights that are all 1 give exactly the same step sizes as not providing
// weights.
TEST(QuantWeights, UnitWeightsMatchNoWeights)
{
  const ojph::ui32 n = 3;
  std::vector<float> w(10, 1.0f);
  ojph::mem_outfile a, b;
  make_headers(a, 2, n, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(0.01f);
    qcd.set_irrev_quant(1, 0.02f, w.data(), w.size());
  });
  make_headers(b, 2, n, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(0.01f);
    qcd.set_irrev_quant(1, 0.02f);
  });
  quant qa = find_quant(a, 1, n), qb = find_quant(b, 1, n);
  ASSERT_TRUE(qa.found && qb.found);
  EXPECT_EQ(qa.delta, qb.delta);
}

///////////////////////////////////////////////////////////////////////////////
// The weights are mapped correctly for a different number of decomposition
// levels (1 level, 4 weights), including the single-component case.
TEST(QuantWeights, FewerDecompositionLevels)
{
  const ojph::ui32 n = 1;
  const float sqrt_w[4] = { 0.0863f, 0.0863f, 0.0263f, 1.0f }; // Cb, 1 level
  std::vector<float> w(sqrt_w, sqrt_w + 4);
  ojph::mem_outfile out;
  make_headers(out, 1, n, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(0, 0.05f, w.data(), w.size());
  });
  quant q = find_quant(out, 0, n);
  ASSERT_TRUE(q.found);
  expect_close(q.delta, expected(0.05f, w, n));
}

///////////////////////////////////////////////////////////////////////////////
// A number of weights that is not 1 + 3 * decomposition levels is an error.
TEST(QuantWeights, WrongNumberOfWeights)
{
  std::vector<float> w(9, 1.0f); // 10 are needed for 3 levels
  ojph::mem_outfile out;
  EXPECT_THROW(make_headers(out, 1, 3, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(0, 0.01f, w.data(), w.size());
  }), std::runtime_error);
}

///////////////////////////////////////////////////////////////////////////////
// A weight that is not positive is an error.
TEST(QuantWeights, InvalidWeights)
{
  std::vector<float> w(10, 1.0f);
  w[4] = 0.0f;
  ojph::mem_outfile out;
  EXPECT_THROW(make_headers(out, 1, 3, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(0, 0.01f, w.data(), w.size());
  }), std::runtime_error);
}

///////////////////////////////////////////////////////////////////////////////
// Weights cannot be used with the reversible transform, where they would
// have no effect.
TEST(QuantWeights, ReversibleRejected)
{
  std::vector<float> w(10, 1.0f);
  ojph::mem_outfile out;
  EXPECT_THROW(make_headers(out, 1, 3, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(0, 0.01f, w.data(), w.size());
  }, true), std::runtime_error);
}

///////////////////////////////////////////////////////////////////////////////
// Mixing set_irrev_quant and set_qfactor.
//
// Three 4:4:4 components are used, as Qfactor needs them. The expected values
// of the components using Qfactor are taken from a header where Qfactor alone
// is set for the whole image, so these tests check how the settings combine,
// not how Qfactor itself is computed.

namespace {

  typedef ojph::param_qcd::comp_type ctype_t;
  const ojph::ui32 NQ = 3;       // decomposition levels
  const float QF = 50.0f;        // Qfactor
  const std::vector<float> UNIT(1 + 3 * NQ, 1.0f);

  // headers where Qfactor is the only thing set
  void qfactor_only(ojph::mem_outfile& out)
  {
    make_headers(out, 3, NQ, [&](ojph::param_qcd qcd) {
      qcd.set_qfactor(QF);
    });
  }

  template<class F>
  void mixed(ojph::mem_outfile& out, F configure)
  { make_headers(out, 3, NQ, configure); }

  // the component's QCC must be identical to the one of the Qfactor-only
  // header
  void expect_like_qfactor(const ojph::mem_outfile& out,
                           const ojph::mem_outfile& ref, int comp)
  {
    quant a = find_quant(out, comp, NQ), b = find_quant(ref, comp, NQ);
    ASSERT_TRUE(a.found && b.found) << "component " << comp;
    EXPECT_EQ(a.delta, b.delta) << "component " << comp;
  }

  void expect_not_like_qfactor(const ojph::mem_outfile& out,
                               const ojph::mem_outfile& ref, int comp)
  {
    quant a = find_quant(out, comp, NQ), b = find_quant(ref, comp, NQ);
    ASSERT_TRUE(a.found && b.found) << "component " << comp;
    EXPECT_NE(a.delta, b.delta) << "component " << comp;
  }

}

///////////////////////////////////////////////////////////////////////////////
// Qfactor set for the whole image, with a plain delta for one component: that
// component uses the delta; the others still use Qfactor.
TEST(QuantMix, GlobalQfactorWithDeltaOnOneComponent)
{
  ojph::mem_outfile ref, out;
  qfactor_only(ref);
  mixed(out, [&](ojph::param_qcd qcd) {
    qcd.set_qfactor(QF);
    qcd.set_irrev_quant(1, 0.02f);
  });
  expect_like_qfactor(out, ref, 0);
  expect_like_qfactor(out, ref, 2);
  expect_not_like_qfactor(out, ref, 1);
  quant q = find_quant(out, 1, NQ);
  expect_close(q.delta, expected(0.02f, UNIT, NQ));
}

///////////////////////////////////////////////////////////////////////////////
// Qfactor set for the whole image, with delta and weights for one component:
// that component uses them; the others still use Qfactor.
TEST(QuantMix, GlobalQfactorWithWeightsOnOneComponent)
{
  std::vector<float> w = CB_W;
  ojph::mem_outfile ref, out;
  qfactor_only(ref);
  mixed(out, [&](ojph::param_qcd qcd) {
    qcd.set_qfactor(QF);
    qcd.set_irrev_quant(2, 0.02f, w.data(), w.size());
  });
  expect_like_qfactor(out, ref, 0);
  expect_like_qfactor(out, ref, 1);
  expect_not_like_qfactor(out, ref, 2);
  quant q = find_quant(out, 2, NQ);
  expect_close(q.delta, expected(0.02f, w, NQ));
}

///////////////////////////////////////////////////////////////////////////////
// Qfactor for a single component, with a global delta: the component uses
// Qfactor, and the others use the global delta through the QCD.
TEST(QuantMix, QfactorOnOneComponentDeltaElsewhere)
{
  ojph::mem_outfile ref, out;
  qfactor_only(ref);
  mixed(out, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(0.01f);
    qcd.set_qfactor(1, ctype_t::OJPH_COMP_CB, QF);
  });
  expect_like_qfactor(out, ref, 1);
  // the others use the QCD
  EXPECT_FALSE(find_quant(out, 0, NQ).found);
  EXPECT_FALSE(find_quant(out, 2, NQ).found);
  quant q = find_quant(out, -1, NQ);
  ASSERT_TRUE(q.found);
  expect_close(q.delta, expected(0.01f, UNIT, NQ));
}

///////////////////////////////////////////////////////////////////////////////
// Qfactor for one component, weights for another, and a global delta for the
// third: each gets its own setting.
TEST(QuantMix, QfactorOnOneComponentWeightsOnAnother)
{
  std::vector<float> w = CB_W;
  ojph::mem_outfile ref, out;
  qfactor_only(ref);
  mixed(out, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(0.01f);
    qcd.set_qfactor(0, ctype_t::OJPH_COMP_Y, QF);
    qcd.set_irrev_quant(2, 0.03f, w.data(), w.size());
  });
  expect_like_qfactor(out, ref, 0);
  EXPECT_FALSE(find_quant(out, 1, NQ).found);
  quant q = find_quant(out, 2, NQ);
  ASSERT_TRUE(q.found);
  expect_close(q.delta, expected(0.03f, w, NQ));
}

///////////////////////////////////////////////////////////////////////////////
// When both are set for the same component, Qfactor wins, whichever was set
// first (a warning is issued about the ignored step size)
TEST(QuantMix, QfactorWinsOverDeltaOnSameComponent)
{
  ojph::mem_outfile ref, a, b, c;
  qfactor_only(ref);
  mixed(a, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(0, 0.02f);
    qcd.set_qfactor(0, ctype_t::OJPH_COMP_Y, QF);
    qcd.set_qfactor(1, ctype_t::OJPH_COMP_CB, QF);
    qcd.set_qfactor(2, ctype_t::OJPH_COMP_CR, QF);
  });
  mixed(b, [&](ojph::param_qcd qcd) {
    qcd.set_qfactor(0, ctype_t::OJPH_COMP_Y, QF);
    qcd.set_irrev_quant(0, 0.02f);
    qcd.set_qfactor(1, ctype_t::OJPH_COMP_CB, QF);
    qcd.set_qfactor(2, ctype_t::OJPH_COMP_CR, QF);
  });
  mixed(c, [&](ojph::param_qcd qcd) { // global settings
    qcd.set_irrev_quant(0.02f);
    qcd.set_qfactor(QF);
  });
  for (int comp = 0; comp < 3; ++comp)
  {
    expect_like_qfactor(a, ref, comp);
    expect_like_qfactor(b, ref, comp);
    expect_like_qfactor(c, ref, comp);
  }
}

///////////////////////////////////////////////////////////////////////////////
// Qfactor and weights for the same component are incompatible, whichever is
// set first. The error is raised when the headers are written.
TEST(QuantMix, QfactorAndWeightsOnSameComponentRejected)
{
  std::vector<float> w(1 + 3 * NQ, 1.0f);
  ojph::mem_outfile out;
  EXPECT_THROW(mixed(out, [&](ojph::param_qcd qcd) {
    qcd.set_qfactor(1, ctype_t::OJPH_COMP_CB, QF);
    qcd.set_irrev_quant(1, 0.02f, w.data(), w.size());
  }), std::runtime_error);
  EXPECT_THROW(mixed(out, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(1, 0.02f, w.data(), w.size());
    qcd.set_qfactor(1, ctype_t::OJPH_COMP_CB, QF);
  }), std::runtime_error);
}

///////////////////////////////////////////////////////////////////////////////
// Mixing set_irrev_quant(delta) and set_irrev_quant(comp_idx, delta, weights,
// len).

namespace {
  const std::vector<float> W1 = CB_W;
  const std::vector<float> W2 = CR_W;
}

///////////////////////////////////////////////////////////////////////////////
// the QCD delta applies to components without their own settings, and the
// weights to the component that has them, whatever the call order
TEST(QuantDeltaWeights, GlobalDeltaWithWeightsOnOneComponent)
{
  ojph::mem_outfile a, b;
  mixed(a, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(0.01f);
    qcd.set_irrev_quant(1, 0.02f, (float*)W1.data(), W1.size());
  });
  mixed(b, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(1, 0.02f, (float*)W1.data(), W1.size());
    qcd.set_irrev_quant(0.01f);
  });
  for (ojph::mem_outfile* out : { &a, &b })
  {
    EXPECT_FALSE(find_quant(*out, 0, NQ).found);
    EXPECT_FALSE(find_quant(*out, 2, NQ).found);
    quant q = find_quant(*out, -1, NQ);
    ASSERT_TRUE(q.found);
    expect_close(q.delta, expected(0.01f, UNIT, NQ));
    q = find_quant(*out, 1, NQ);
    ASSERT_TRUE(q.found);
    expect_close(q.delta, expected(0.02f, W1, NQ));
  }
  // the order does not matter
  EXPECT_EQ(find_quant(a, 1, NQ).delta, find_quant(b, 1, NQ).delta);
  EXPECT_EQ(find_quant(a, -1, NQ).delta, find_quant(b, -1, NQ).delta);
}

///////////////////////////////////////////////////////////////////////////////
// the component's delta is its own; the QCD delta does not leak into it
TEST(QuantDeltaWeights, ComponentDeltaIndependentOfGlobalDelta)
{
  ojph::mem_outfile a, b;
  mixed(a, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(0.01f);
    qcd.set_irrev_quant(1, 0.02f, (float*)W1.data(), W1.size());
  });
  mixed(b, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(0.07f);
    qcd.set_irrev_quant(1, 0.02f, (float*)W1.data(), W1.size());
  });
  EXPECT_EQ(find_quant(a, 1, NQ).delta, find_quant(b, 1, NQ).delta);
  EXPECT_NE(find_quant(a, -1, NQ).delta, find_quant(b, -1, NQ).delta);
}

///////////////////////////////////////////////////////////////////////////////
// without a QCD delta, the default one is used for the other components,
// while the weighted component is unaffected
TEST(QuantDeltaWeights, DefaultGlobalDeltaWithWeights)
{
  ojph::mem_outfile out;
  mixed(out, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(2, 0.02f, (float*)W1.data(), W1.size());
  });
  quant q = find_quant(out, 2, NQ);
  ASSERT_TRUE(q.found);
  expect_close(q.delta, expected(0.02f, W1, NQ));
  EXPECT_FALSE(find_quant(out, 0, NQ).found);
  EXPECT_FALSE(find_quant(out, 1, NQ).found);
  // 8 bits: default delta is 1 / 2^8
  quant g = find_quant(out, -1, NQ);
  ASSERT_TRUE(g.found);
  expect_close(g.delta, expected(1.0f / 256.0f, UNIT, NQ));
}

///////////////////////////////////////////////////////////////////////////////
// Two components with different weights and deltas, plus a global delta for
// the third: each gets its own step sizes, and they do not interfere.
TEST(QuantDeltaWeights, DifferentWeightsOnDifferentComponents)
{
  ojph::mem_outfile out;
  mixed(out, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(0.01f);
    qcd.set_irrev_quant(0, 0.02f, (float*)W1.data(), W1.size());
    qcd.set_irrev_quant(2, 0.03f, (float*)W2.data(), W2.size());
  });
  quant q0 = find_quant(out, 0, NQ), q2 = find_quant(out, 2, NQ);
  ASSERT_TRUE(q0.found && q2.found);
  expect_close(q0.delta, expected(0.02f, W1, NQ));
  expect_close(q2.delta, expected(0.03f, W2, NQ));
  EXPECT_FALSE(find_quant(out, 1, NQ).found);
  expect_close(find_quant(out, -1, NQ).delta, expected(0.01f, UNIT, NQ));
}

///////////////////////////////////////////////////////////////////////////////
// setting the weights again for a component replaces the previous ones
TEST(QuantDeltaWeights, WeightsReplacedOnSecondCall)
{
  ojph::mem_outfile out;
  mixed(out, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(1, 0.02f, (float*)W1.data(), W1.size());
    qcd.set_irrev_quant(1, 0.04f, (float*)W2.data(), W2.size());
  });
  expect_close(find_quant(out, 1, NQ).delta, expected(0.04f, W2, NQ));
}

///////////////////////////////////////////////////////////////////////////////
// setting just a delta for a component that has weights keeps the weights
TEST(QuantDeltaWeights, ComponentDeltaAfterWeights)
{
  ojph::mem_outfile out;
  mixed(out, [&](ojph::param_qcd qcd) {
    qcd.set_irrev_quant(1, 0.02f, (float*)W1.data(), W1.size());
    qcd.set_irrev_quant(1, 0.04f);
  });
  expect_close(find_quant(out, 1, NQ).delta, expected(0.04f, W1, NQ));
}
