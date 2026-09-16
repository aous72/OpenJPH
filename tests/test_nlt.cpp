//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2019, Aous Naman
// Copyright (c) 2019, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2019, The University of New South Wales, Australia
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
// File: test_nlt.cpp
//***************************************************************************/
// Tests of the non-linearity point transformation (NLT) marker segment, for
// the three nonlinearities this library supports, namely
//
//   OJPH_NLT_LUT_STYLE_NLT              = 2 -- LUT style
//   OJPH_NLT_BINARY_COMPLEMENT_NLT      = 3 -- binary complement (SMAG)
//   OJPH_NLT_BINARY_COMPLEMENT_PLUS_LUT = 4 -- binary complement plus a LUT
//
// The tests drive the library directly rather than the ojph_compress and
// ojph_expand executables, because only the library lets a caller choose the
// nonlinearity and describe the look-up table it uses.  Encoding and
// decoding are both exercised: each test encodes an image with a
// nonlinearity in effect and then decodes the codestream it produced,
// comparing the decoded samples with the samples that were fed in.
//
// The nonlinearity is a point transformation applied before the wavelet
// transform when encoding, and undone after the inverse wavelet transform
// when decoding.  Type 3 is exactly reversible, so a round trip through a
// reversible (5/3) codestream returns the samples bit for bit.  Types 2 and 4
// describe the transformation with a look-up table, which is an
// approximation, so those round trips are only approximately lossless.
//***************************************************************************/

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "ojph_arch.h"
#include "ojph_mem.h"
#include "ojph_message.h"
#include "ojph_file.h"
#include "ojph_codestream.h"
#include "ojph_params.h"
#include "gtest/gtest.h"

using namespace ojph;

#ifdef OJPH_OS_WINDOWS
#define REF_FILE_DIR ".\\jp2k_test_codestreams\\openjph\\references\\"
#define OUT_FILE_DIR ".\\"
#else
#define REF_FILE_DIR "./jp2k_test_codestreams/openjph/references/"
#define OUT_FILE_DIR "./"
#endif

// the JPEG2000 Part 2 marker segment that carries the nonlinearity; see
// JP2K_MARKER::NLT
#define NLT_MARKER_HIGH  0xFF
#define NLT_MARKER_LOW   0x76

namespace {

  ///////////////////////////////////////////////////////////////////////////
  // The look-up table that ojph_compress installs for floating point (.pfm)
  // images.  It is repeated here so that this test does not depend on the
  // application.  Dmin and Dmax are the smallest and the largest 32 bit
  // pattern the samples of an image can take, the points are the look-up
  // table itself, and the precision of a point (pt_val) is 32 bits.
  ///////////////////////////////////////////////////////////////////////////
  const ui32 pfmLutNumPoints = 152;
  const ui32 pfmLutDmin = 67109888;
  const ui32 pfmLutDmax = 4227857408;
  const ui32 pfmLutPoints[pfmLutNumPoints] = {
    67109888,   88737098,   106890847,  122160968,  135661590,  157092189,
    175049327,  190122837,  204213292,  225447280,  243207807,  258084706,
    272764994,  293736834,  311300750,  326046575,  341316696,  362026388,
    379393693,  394008444,  409802861,  430315942,  447486636,  461904776,
    478289026,  498605496,  515579579,  529866645,  546775191,  566829513,
    583672522,  597762977,  615261356,  635053530,  651699928,  665659309,
    683681984,  703343084,  719792871,  733555641,  752102612,  771501564,
    787820277,  801451973,  820523240,  839725581,  855847683,  869348305,
    888943868,  907949598,  923875089,  937179100,  957364496,  976108078,
    991836958,  1005075432, 1025719587, 1044266558, 1059864364, 1072906227,
    1094074678, 1112425038, 1127891770, 1140802559, 1163281750, 1183532683,
    1201620895, 1227180325, 1254836939, 1281838183, 1321029309, 1362907452,
    1412912183, 1485985938, 1596677931, 1830579484, 2464387812, 2698289365,
    2808981358, 2882055113, 2932059844, 2973937987, 3013129113, 3040130357,
    3067786971, 3093346401, 3111434613, 3131685546, 3154164737, 3167075526,
    3182542258, 3200892618, 3222061069, 3235102932, 3250700738, 3269247709,
    3289891864, 3303130338, 3318859218, 3337602800, 3357788196, 3371092207,
    3387017698, 3406023428, 3425618991, 3439119613, 3455241715, 3474444056,
    3493515323, 3507147019, 3523465732, 3542864684, 3561411655, 3575174425,
    3591624212, 3611285312, 3629307987, 3643267368, 3659913766, 3679705940,
    3697204319, 3711294774, 3728137783, 3748192105, 3765100651, 3779387717,
    3796361800, 3816678270, 3833062520, 3847480660, 3864651354, 3885164435,
    3900958852, 3915573603, 3932940908, 3953650600, 3968920721, 3983666546,
    4001230462, 4022202302, 4036882590, 4051759489, 4069520016, 4090754004,
    4104844459, 4119917969, 4137875107, 4159305706, 4172806328, 4188076449,
    4206230198, 4227857408};

  ///////////////////////////////////////////////////////////////////////////
  // A look-up table that runs over the whole range of 32 bit patterns.  The
  // curve is a straight line, hence this table describes an identity
  // transformation and a round trip through it has to be as accurate as a
  // round trip without a nonlinearity.
  ///////////////////////////////////////////////////////////////////////////
  const ui32 identityLutNumPoints = 129;
  ui32 identityLutPoints[identityLutNumPoints];
  bool identityLutReady = false;

  void prepare_identity_lut()
  {
    if (identityLutReady)
      return;
    for (ui32 i = 0; i < identityLutNumPoints; ++i)
      identityLutPoints[i] =
        (ui32)(((double)i / (identityLutNumPoints - 1)) * 4294967295.0);
    identityLutReady = true;
  }

  ///////////////////////////////////////////////////////////////////////////
  // An image held in memory.  A sample is the 32 bit pattern of an image
  // sample, which is what a .pfm file holds; for a floating point image these
  // are the IEEE-754 single precision bit patterns of the samples.
  ///////////////////////////////////////////////////////////////////////////
  struct test_image
  {
    ui32 width = 0, height = 0, num_comps = 0;
    std::vector<std::vector<si32> > samples;    // one entry per component

    size_t num_samples() const
    { return (size_t)width * height * num_comps; }
  };

  ///////////////////////////////////////////////////////////////////////////
  // How well a decoded image agrees with the image it was decoded from
  ///////////////////////////////////////////////////////////////////////////
  struct comparison
  {
    long long max_abs_error = 0;   // largest error in 32 bit pattern units
    double mean_abs_error = 0.0;
    double max_rel_error = 0.0;    // largest error relative to the sample
    double mean_rel_error = 0.0;
    size_t num_bit_exact = 0;      // samples that came back unchanged
    size_t num_samples = 0;

    bool is_bit_exact() const { return num_bit_exact == num_samples; }
  };

  ///////////////////////////////////////////////////////////////////////////
  // Reads a pfm file.  The samples of a pfm file are 32 bit floating point
  // numbers, and they are handed to the library as the 32 bit patterns they
  // are stored as; the library works on those patterns.
  ///////////////////////////////////////////////////////////////////////////
  bool load_pfm(const std::string& filename, test_image& img)
  {
    FILE* f = fopen(filename.c_str(), "rb");
    if (f == NULL)
      return false;

    char magic[3] = { 0, 0, 0 };
    if (fscanf(f, "%2s", magic) != 1 ||
        (magic[1] != 'f' && magic[1] != 'F'))
    {
      fclose(f);
      return false;
    }

    int w = 0, h = 0;
    float scale = 0.0f;
    if (fscanf(f, "%d %d %f", &w, &h, &scale) != 3 || w <= 0 || h <= 0)
    {
      fclose(f);
      return false;
    }
    fgetc(f);    // the single white space that follows the header

    const bool swap = scale > 0.0f;   // a positive scale means big endian

    img.width = (ui32)w;
    img.height = (ui32)h;
    img.num_comps = magic[1] == 'f' ? 1 : 3;
    img.samples.assign(img.num_comps, std::vector<si32>((size_t)w * h, 0));

    std::vector<ui32> line((size_t)w * img.num_comps, 0);
    for (int y = h - 1; y >= 0; --y)    // pfm stores the last line first
    {
      if (fread(&line[0], sizeof(ui32), (size_t)w * img.num_comps, f)
          != (size_t)w * img.num_comps)
      {
        fclose(f);
        return false;
      }
      for (int x = 0; x < w; ++x)
        for (ui32 c = 0; c < img.num_comps; ++c)
        {
          ui32 u = line[(size_t)x * img.num_comps + c];
          if (swap)
            u = (u >> 24) | ((u >> 8) & 0x0000FF00u) |
                ((u << 8) & 0x00FF0000u) | (u << 24);
          img.samples[c][(size_t)y * w + x] = (si32)u;
        }
    }

    fclose(f);
    return true;
  }

  ///////////////////////////////////////////////////////////////////////////
  // Locates one of the images the tests need.  The images belong to the
  // repository that stores the test images, jp2k_test_codestreams.  The other
  // directories looked in are a convenience, so that an image can be tested
  // before it has been added to that repository.  An empty string is returned
  // when the image cannot be found.
  ///////////////////////////////////////////////////////////////////////////
  std::string find_test_image(const std::string& name)
  {
    const char* dirs[] = { REF_FILE_DIR };
    for (size_t i = 0; i < sizeof(dirs) / sizeof(dirs[0]); ++i)
    {
      std::string path = std::string(dirs[i]) + name;
      FILE* f = fopen(path.c_str(), "rb");
      if (f != NULL)
      {
        fclose(f);
        return path;
      }
    }
    return std::string();
  }

  ///////////////////////////////////////////////////////////////////////////
  // The two images the tests use: random.pfm is a single component image of
  // floating point samples that are large and of both signs, and rgb.pfm has
  // three components.
  ///////////////////////////////////////////////////////////////////////////
  const char* const image_names[2] = { "random.pfm", "rgb.pfm" };
  const size_t num_images = sizeof(image_names) / sizeof(image_names[0]);

  ///////////////////////////////////////////////////////////////////////////
  // Loads both images; returns the names of those that could not be loaded.
  ///////////////////////////////////////////////////////////////////////////
  std::string load_test_images(test_image images[num_images])
  {
    std::string missing;
    for (size_t i = 0; i < num_images; ++i)
    {
      std::string path = find_test_image(image_names[i]);
      if (path.empty() || !load_pfm(path, images[i]))
      {
        if (!missing.empty())
          missing += " and ";
        missing += image_names[i];
      }
    }
    return missing;
  }

  ///////////////////////////////////////////////////////////////////////////
  // The message used when the test images are not available
  ///////////////////////////////////////////////////////////////////////////
  std::string missing_images_message(const std::string& missing)
  {
    return std::string("the test image(s) ") + missing + " could not be "
      "found; they belong to the repository that stores the test images, "
      "jp2k_test_codestreams, in the folder openjph/references, and the tests "
      "that use them are skipped until they are added there";
  }

  ///////////////////////////////////////////////////////////////////////////
  // Fills an image with a ramp that covers the range of 32 bit patterns.
  // Samples whose magnitude is very small are left out, because the parts of
  // the transformation that work with floating point numbers cannot resolve
  // the samples close to the middle of the range.
  ///////////////////////////////////////////////////////////////////////////
  void make_synthetic_image(test_image& img, ui32 width, ui32 height)
  {
    img.width = width;
    img.height = height;
    img.num_comps = 1;
    img.samples.assign(1, std::vector<si32>((size_t)width * height, 0));

    // the limits below sit inside the range the look-up table for .pfm
    // images covers, so that no sample is clamped by it
    const si64 lowest = -2070000000LL;
    const si64 highest = 2070000000LL;
    const si64 gap = 1LL << 20;    // the samples around zero are skipped
    const si64 half = (si64)((size_t)width * height / 2);
    std::vector<si32>& s = img.samples[0];
    for (size_t i = 0; i < s.size(); ++i)
    {
      if (i < (size_t)half)
        s[i] = (si32)(gap + (highest - gap) * (si64)i / (half - 1));
      else
        s[i] = (si32)(-gap +
          (lowest + gap) * (si64)(i - (size_t)half) / (half - 1));
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  // The nonlinearity a test asks for, together with the coding settings used
  // with it
  ///////////////////////////////////////////////////////////////////////////
  struct nlt_setting
  {
    ui8 type;            // 0, 2, 3 or 4
    bool use_pfm_lut;    // true: the look-up table used for .pfm images
    bool reversible;     // false: the 9/7 wavelet, true: the 5/3 wavelet
    float qstep;         // the quantization step used with the 9/7 wavelet
  };

  ///////////////////////////////////////////////////////////////////////////
  // The most common setting used below, the 9/7 wavelet with a quantization
  // step of 1/16384, which is the smallest step ojph_compress uses for .pfm
  // images.
  ///////////////////////////////////////////////////////////////////////////
  float default_qstep() { return 1.0f / 16384.0f; }

  ///////////////////////////////////////////////////////////////////////////
  // Installs the nonlinearity on a codestream that is about to be written
  ///////////////////////////////////////////////////////////////////////////
  void apply_nlt(codestream& cs, const nlt_setting& setting)
  {
    param_nlt nlt = cs.access_nlt();
    if (setting.type == param_nlt::OJPH_NLT_BINARY_COMPLEMENT_NLT)
    {
      nlt.set_nonlinear_transform(param_nlt::ALL_COMPS,
        param_nlt::OJPH_NLT_BINARY_COMPLEMENT_NLT);
    }
    else if (setting.type == param_nlt::OJPH_NLT_LUT_STYLE_NLT ||
             setting.type == param_nlt::OJPH_NLT_BINARY_COMPLEMENT_PLUS_LUT)
    {
      ui32 d_min, d_max, num_points;
      void* points;
      if (setting.use_pfm_lut)
      {
        d_min = pfmLutDmin;
        d_max = pfmLutDmax;
        num_points = pfmLutNumPoints;
        points = (void*)pfmLutPoints;
      }
      else
      {  // the identity table runs over the whole range of 32 bit patterns
        prepare_identity_lut();
        d_min = 0;
        d_max = 0xFFFFFFFFu;
        num_points = identityLutNumPoints;
        points = (void*)identityLutPoints;
      }
      nlt.set_nonlinear_transform(param_nlt::ALL_COMPS, 32, true,
        d_min, d_max, 32, (ui16)num_points, points, setting.type);
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  // Encodes an image with the requested nonlinearity in effect
  ///////////////////////////////////////////////////////////////////////////
  void encode_image(const std::string& filename, const test_image& img,
                    const nlt_setting& setting)
  {
    codestream cs;

    param_siz siz = cs.access_siz();
    siz.set_image_extent(point(img.width, img.height));
    siz.set_num_components(img.num_comps);
    for (ui32 c = 0; c < img.num_comps; ++c)
      siz.set_component(c, point(1, 1), 32, true);
    siz.set_image_offset(point(0, 0));
    siz.set_tile_size(size(img.width, img.height));
    siz.set_tile_offset(point(0, 0));

    param_cod cod = cs.access_cod();
    cod.set_num_decomposition(5);
    cod.set_block_dims(64, 64);
    cod.set_color_transform(false);
    cod.set_reversible(setting.reversible);
    if (!setting.reversible)
      cs.access_qcd().set_irrev_quant(setting.qstep);

    apply_nlt(cs, setting);

    cs.set_planar(true);

    j2c_outfile file;
    file.open(filename.c_str());
    cs.write_headers(&file);

    ui32 next_comp;
    line_buf* cur_line = cs.exchange(NULL, next_comp);
    ASSERT_TRUE(cur_line != NULL) << filename;
    for (ui32 c = 0; c < img.num_comps; ++c)
      for (ui32 y = 0; y < img.height; ++y)
      {
        ASSERT_EQ(next_comp, c) << filename;
        for (ui32 x = 0; x < img.width; ++x)
          cur_line->i32[x] = img.samples[c][(size_t)y * img.width + x];
        cur_line = cs.exchange(cur_line, next_comp);
      }

    cs.flush();
    cs.close();
  }

  ///////////////////////////////////////////////////////////////////////////
  // Decodes an image; also reports the nonlinearity found in the codestream
  ///////////////////////////////////////////////////////////////////////////
  void decode_image(const std::string& filename, test_image& img,
                    bool* has_nlt, ui8* nlt_type, ui8* nlt_bit_depth,
                    bool* nlt_is_signed)
  {
    codestream cs;
    j2c_infile file;
    file.open(filename.c_str());
    cs.read_headers(&file);

    if (has_nlt != NULL || nlt_type != NULL || nlt_bit_depth != NULL ||
        nlt_is_signed != NULL)
    {
      ui8 bit_depth = 0, type = 0;
      bool is_signed = false;
      bool found = cs.access_nlt().get_nonlinear_transform(
        param_nlt::ALL_COMPS, bit_depth, is_signed, type);
      if (has_nlt != NULL) *has_nlt = found;
      if (nlt_type != NULL) *nlt_type = type;
      if (nlt_bit_depth != NULL) *nlt_bit_depth = bit_depth;
      if (nlt_is_signed != NULL) *nlt_is_signed = is_signed;
    }

    param_siz siz = cs.access_siz();
    img.num_comps = siz.get_num_components();
    img.width = siz.get_recon_width(0);
    img.height = siz.get_recon_height(0);
    img.samples.assign(img.num_comps,
      std::vector<si32>((size_t)img.width * img.height, 0));

    cs.restrict_input_resolution(0, 0);
    cs.set_planar(true);
    cs.create();

    for (ui32 c = 0; c < img.num_comps; ++c)
      for (ui32 y = 0; y < img.height; ++y)
      {
        ui32 comp_num = 0;
        line_buf* line = cs.pull(comp_num);
        ASSERT_TRUE(line != NULL) << filename;
        ASSERT_EQ(comp_num, c) << filename;
        for (ui32 x = 0; x < img.width; ++x)
          img.samples[c][(size_t)y * img.width + x] = line->i32[x];
      }

    cs.close();
  }

  ///////////////////////////////////////////////////////////////////////////
  // Compares the image that was encoded with the image that was decoded.  The
  // samples are compared both as the 32 bit patterns they are and as the
  // floating point numbers those patterns stand for.
  ///////////////////////////////////////////////////////////////////////////
  comparison compare_images(const test_image& before, const test_image& after)
  {
    comparison result;

    double peak = 0.0;
    for (ui32 c = 0; c < before.num_comps; ++c)
      for (size_t i = 0; i < before.samples[c].size(); ++i)
      {
        float v;
        memcpy(&v, &before.samples[c][i], sizeof(float));
        peak = std::max(peak, (double)fabs(v));
      }
    // a relative error is not meaningful for a sample that is essentially
    // zero; the floor below keeps such samples from dominating the result
    const double floor = std::max(peak * 1e-5, 1e-30);

    double sum_abs = 0.0, sum_rel = 0.0;
    for (ui32 c = 0; c < before.num_comps; ++c)
      for (size_t i = 0; i < before.samples[c].size(); ++i)
      {
        si32 a = before.samples[c][i], b = after.samples[c][i];
        si64 err = (si64)a - (si64)b;
        if (err < 0)
          err = -err;
        result.max_abs_error = std::max(result.max_abs_error, (long long)err);
        sum_abs += (double)err;

        float av, bv;
        memcpy(&av, &a, sizeof(float));
        memcpy(&bv, &b, sizeof(float));
        double rel = fabs((double)av - (double)bv) /
          std::max((double)fabs(av), floor);
        result.max_rel_error = std::max(result.max_rel_error, rel);
        sum_rel += rel;

        if (a == b)
          ++result.num_bit_exact;
        ++result.num_samples;
      }

    result.mean_abs_error = sum_abs / (double)result.num_samples;
    result.mean_rel_error = sum_rel / (double)result.num_samples;
    return result;
  }

  ///////////////////////////////////////////////////////////////////////////
  // Encodes an image and decodes it again, and reports how well the decoded
  // image agrees with the image that was encoded
  ///////////////////////////////////////////////////////////////////////////
  comparison round_trip(const std::string& tag, const test_image& img,
                        const nlt_setting& setting)
  {
    const std::string filename = std::string(OUT_FILE_DIR) + tag + ".j2c";
    encode_image(filename, img, setting);

    test_image decoded;
    decode_image(filename, decoded, NULL, NULL, NULL, NULL);

    EXPECT_EQ(decoded.width, img.width) << tag;
    EXPECT_EQ(decoded.height, img.height) << tag;
    EXPECT_EQ(decoded.num_comps, img.num_comps) << tag;

    return compare_images(img, decoded);
  }

  ///////////////////////////////////////////////////////////////////////////
  // Tells whether a codestream holds an NLT marker segment
  ///////////////////////////////////////////////////////////////////////////
  bool has_nlt_marker(const std::string& filename)
  {
    FILE* f = fopen(filename.c_str(), "rb");
    if (f == NULL)
      return false;

    bool found = false;
    int prev = EOF, cur;
    while ((cur = fgetc(f)) != EOF)
    {
      if (prev == NLT_MARKER_HIGH && cur == NLT_MARKER_LOW)
      {
        found = true;
        break;
      }
      prev = cur;
    }
    fclose(f);
    return found;
  }

  ///////////////////////////////////////////////////////////////////////////
  // Collects the messages the library issues, so that a test can tell the
  // message it expects apart from an unrelated failure.
  ///////////////////////////////////////////////////////////////////////////
  struct warning_collector : public message_warning
  {
    int num_messages = 0;
    int last_code = 0;

    virtual void operator() (int warn_code, const char* file_name,
      int line_num, const char* fmt, ...)
    {
      (void)file_name;
      (void)line_num;
      (void)fmt;
      ++num_messages;
      last_code = warn_code;
    }
  };

  struct error_collector : public message_error
  {
    int num_messages = 0;
    int last_code = 0;

    virtual void operator() (int error_code, const char* file_name,
      int line_num, const char* fmt, ...)
    {
      (void)file_name;
      (void)line_num;
      (void)fmt;
      ++num_messages;
      last_code = error_code;
      throw std::runtime_error("ojph error");   // as the library does
    }
  };

  struct message_capture
  {
    warning_collector warnings;
    error_collector errors;
    message_warning* previous_warning = NULL;
    message_error* previous_error = NULL;

    message_capture()
    {
      previous_warning = get_warning();
      previous_error = get_error();
      configure_warning(&warnings);
      configure_error(&errors);
    }

    ~message_capture()
    {
      configure_warning(previous_warning);
      configure_error(previous_error);
    }
  };

  ///////////////////////////////////////////////////////////////////////////
  // Inserts an NLT marker segment for a LUT style nonlinearity into a
  // codestream, right before its first tile part.  The library refuses to
  // write that combination, so a codestream that carries it has to be put
  // together here, the way another encoder may have produced it.
  ///////////////////////////////////////////////////////////////////////////
  bool insert_lut_nlt_marker(const std::string& filename, ui8 nlt_type)
  {
    FILE* f = fopen(filename.c_str(), "rb");
    if (f == NULL)
      return false;

    std::vector<ui8> data;
    ui8 buffer[4096];
    size_t num_read;
    while ((num_read = fread(buffer, 1, sizeof(buffer), f)) > 0)
      data.insert(data.end(), buffer, buffer + num_read);
    fclose(f);

    // the marker goes just before the first tile part
    size_t sot = data.size();
    for (size_t i = 0; i + 1 < data.size(); ++i)
      if (data[i] == 0xFF && data[i + 1] == 0x90)
      {
        sot = i;
        break;
      }
    if (sot == data.size())
      return false;

    const ui16 num_points = 17;
    const ui32 d_min = 0, d_max = 0xFFFFFFFFu;
    const ui32 length = 17u + 4u * (ui32)num_points;  // as the library does

    std::vector<ui8> marker;
    marker.push_back(0xFF);
    marker.push_back(0x76);                       // NLT
    marker.push_back((ui8)(length >> 8));
    marker.push_back((ui8)(length & 0xFF));
    marker.push_back(0xFF);
    marker.push_back(0xFF);                       // Cnlt = ALL_COMPS
    marker.push_back(0x9F);                       // BDnlt: 32 bits, signed
    marker.push_back(nlt_type);
    marker.push_back((ui8)((num_points - 1) >> 8));    // the field carries
    marker.push_back((ui8)((num_points - 1) & 0xFF));  // one point less
    for (int i = 3; i >= 0; --i)
      marker.push_back((ui8)(d_min >> (8 * i)));
    for (int i = 3; i >= 0; --i)
      marker.push_back((ui8)(d_max >> (8 * i)));
    marker.push_back(32);                         // pt_val
    for (ui32 i = 0; i < (ui32)num_points; ++i)
    {
      ui32 point =
        (ui32)(((double)i / (num_points - 1)) * 4294967295.0);
      for (int b = 3; b >= 0; --b)
        marker.push_back((ui8)(point >> (8 * b)));
    }

    data.insert(data.begin() + (long)sot, marker.begin(), marker.end());

    f = fopen(filename.c_str(), "wb");
    if (f == NULL)
      return false;
    bool ok = fwrite(&data[0], 1, data.size(), f) == data.size();
    fclose(f);
    return ok;
  }

  ///////////////////////////////////////////////////////////////////////////
  // Encodes a two component image in which component 1 is coded with the
  // irreversible wavelet while component 0 keeps the reversible one; a COC
  // marker segment is what lets the two differ.  The LUT style nonlinearity
  // is attached to the component the caller names.
  ///////////////////////////////////////////////////////////////////////////
  void encode_two_component_image(const std::string& filename,
                                  const test_image& img, ui32 nlt_comp)
  {
    codestream cs;

    param_siz siz = cs.access_siz();
    siz.set_image_extent(point(img.width, img.height));
    siz.set_num_components(2);
    for (ui32 c = 0; c < 2; ++c)
      siz.set_component(c, point(1, 1), 32, true);
    siz.set_image_offset(point(0, 0));
    siz.set_tile_size(size(img.width, img.height));
    siz.set_tile_offset(point(0, 0));

    param_cod cod = cs.access_cod();
    cod.set_num_decomposition(5);
    cod.set_block_dims(64, 64);
    cod.set_color_transform(false);
    cod.set_reversible(true);              // component 0 stays reversible
    cod.set_reversible(1, false);          // component 1 is irreversible
    cs.access_qcd().set_irrev_quant(default_qstep());

    cs.access_nlt().set_nonlinear_transform(nlt_comp, 32, true,
      pfmLutDmin, pfmLutDmax, 32, (ui16)pfmLutNumPoints, (void*)pfmLutPoints,
      param_nlt::OJPH_NLT_LUT_STYLE_NLT);

    cs.set_planar(true);

    j2c_outfile file;
    file.open(filename.c_str());
    cs.write_headers(&file);

    ui32 next_comp;
    line_buf* cur_line = cs.exchange(NULL, next_comp);
    ASSERT_TRUE(cur_line != NULL) << filename;
    for (ui32 c = 0; c < 2; ++c)
      for (ui32 y = 0; y < img.height; ++y)
      {
        ASSERT_EQ(next_comp, c) << filename;
        for (ui32 x = 0; x < img.width; ++x)
          cur_line->i32[x] = img.samples[0][(size_t)y * img.width + x];
        cur_line = cs.exchange(cur_line, next_comp);
      }

    cs.flush();
    cs.close();
  }

} // anonymous namespace

///////////////////////////////////////////////////////////////////////////////
// The NLT marker segment has to survive a write and a read of the codestream
// headers, for each of the three nonlinearities, and the nonlinearity and the
// format of the samples the nonlinearity works on have to come back unchanged.
///////////////////////////////////////////////////////////////////////////////
TEST(NltTest, MarkerSegmentRoundTrip)
{
  test_image img;
  make_synthetic_image(img, 64, 64);

  const ui8 types[3] = { param_nlt::OJPH_NLT_LUT_STYLE_NLT,
                         param_nlt::OJPH_NLT_BINARY_COMPLEMENT_NLT,
                         param_nlt::OJPH_NLT_BINARY_COMPLEMENT_PLUS_LUT };
  for (size_t i = 0; i < 3; ++i)
  {
    nlt_setting setting;
    setting.type = types[i];
    setting.use_pfm_lut = true;
    setting.reversible = false;
    setting.qstep = default_qstep();

    const std::string filename = OUT_FILE_DIR "nlt_marker.j2c";
    encode_image(filename, img, setting);

    bool has_nlt = false, is_signed = false;
    ui8 type = 0, bit_depth = 0;
    test_image decoded;
    decode_image(filename, decoded, &has_nlt, &type, &bit_depth, &is_signed);

    EXPECT_TRUE(has_nlt) << "nonlinearity " << (int)types[i];
    EXPECT_EQ((int)type, (int)types[i]) << "nonlinearity " << (int)types[i];
    EXPECT_EQ((int)bit_depth, 32) << "nonlinearity " << (int)types[i];
    EXPECT_TRUE(is_signed) << "nonlinearity " << (int)types[i];
    EXPECT_TRUE(has_nlt_marker(filename)) << "nonlinearity " << (int)types[i];
  }
}

///////////////////////////////////////////////////////////////////////////////
// A codestream that does not ask for a nonlinearity must not hold an NLT
// marker segment, and must not report a nonlinearity when its headers are
// read back.
///////////////////////////////////////////////////////////////////////////////
TEST(NltTest, NoMarkerSegmentWithoutNlt)
{
  test_image img;
  make_synthetic_image(img, 64, 64);

  nlt_setting setting;
  setting.type = param_nlt::OJPH_NLT_NO_NLT;
  setting.use_pfm_lut = true;
  setting.reversible = false;
  setting.qstep = default_qstep();

  const std::string filename = OUT_FILE_DIR "nlt_none.j2c";
  encode_image(filename, img, setting);

  bool has_nlt = true;
  test_image decoded;
  decode_image(filename, decoded, &has_nlt, NULL, NULL, NULL);

  EXPECT_FALSE(has_nlt);
  EXPECT_FALSE(has_nlt_marker(filename));
}

///////////////////////////////////////////////////////////////////////////////
// With an identity look-up table the LUT style nonlinearities, 2 and 4, do
// not change the samples, so a round trip through either of them has to be as
// accurate as a round trip that does not use a nonlinearity.  This isolates
// the nonlinearity from the accuracy of the table itself: it fails if the
// table is not inverted correctly, or if the samples are not exchanged in the
// format the nonlinearity expects.
///////////////////////////////////////////////////////////////////////////////
TEST(NltTest, LutStyleWithIdentityLutKeepsSamples)
{
  test_image img;
  make_synthetic_image(img, 128, 128);

  nlt_setting baseline;
  baseline.type = param_nlt::OJPH_NLT_NO_NLT;
  baseline.use_pfm_lut = false;
  baseline.reversible = false;
  baseline.qstep = 1e-5f;
  comparison plain = round_trip("nlt_identity_plain", img, baseline);

  const ui8 types[2] = { param_nlt::OJPH_NLT_LUT_STYLE_NLT,
                         param_nlt::OJPH_NLT_BINARY_COMPLEMENT_PLUS_LUT };
  for (size_t i = 0; i < 2; ++i)
  {
    nlt_setting setting = baseline;
    setting.type = types[i];
    const std::string tag = "nlt_identity_" + std::to_string((int)types[i]);
    comparison nlt = round_trip(tag, img, setting);

    // the identity table only adds the rounding of a transformation that does
    // nothing, so the average error has to stay in the neighbourhood of the
    // error of the round trip that does not use a nonlinearity
    EXPECT_LE(nlt.mean_abs_error, 3.0 * plain.mean_abs_error + 16.0)
      << "nonlinearity " << (int)types[i] << ": mean absolute error "
      << nlt.mean_abs_error << " against " << plain.mean_abs_error
      << " for a round trip without a nonlinearity";
    EXPECT_LT(nlt.max_rel_error, 0.01)
      << "nonlinearity " << (int)types[i] << " with an identity table";
  }
}

///////////////////////////////////////////////////////////////////////////////
// Type 3 is a reversible point transformation, so with the reversible (5/3)
// wavelet the samples of both test images come back bit for bit.
///////////////////////////////////////////////////////////////////////////////
TEST(NltTest, BinaryComplementIsLossless)
{
  test_image images[num_images];
  std::string missing = load_test_images(images);
  // if (!missing.empty())
  // {
  //   GTEST_SKIP() << missing_images_message(missing);
  //   return;
  // }

  for (size_t i = 0; i < num_images; ++i)
  {
    nlt_setting setting;
    setting.type = param_nlt::OJPH_NLT_BINARY_COMPLEMENT_NLT;
    setting.use_pfm_lut = false;
    setting.reversible = true;
    setting.qstep = default_qstep();

    const std::string tag = std::string("nlt_lossless_") + image_names[i];
    comparison result = round_trip(tag, images[i], setting);

    EXPECT_TRUE(result.is_bit_exact()) << image_names[i] << ": "
      << (result.num_samples - result.num_bit_exact) << " of "
      << result.num_samples << " samples changed; the largest change is "
      << result.max_abs_error << " in a 32 bit pattern";
  }
}

///////////////////////////////////////////////////////////////////////////////
// Round trips of the test images with each nonlinearity, using the look-up
// table that ojph_compress installs for .pfm images.  The 9/7 wavelet is
// lossy, so the decoded image is compared with a round trip of the same image
// that does not use a nonlinearity: a nonlinearity must not add much to the
// error of that round trip, and the samples have to come back to within a
// fraction of a percent of their value.
///////////////////////////////////////////////////////////////////////////////
TEST(NltTest, RoundTripOfTestImages)
{
  test_image images[num_images];
  std::string missing = load_test_images(images);
  // if (!missing.empty())
  // {
  //   GTEST_SKIP() << missing_images_message(missing);
  //   return;
  // }

  const ui8 types[3] = { param_nlt::OJPH_NLT_LUT_STYLE_NLT,
                         param_nlt::OJPH_NLT_BINARY_COMPLEMENT_NLT,
                         param_nlt::OJPH_NLT_BINARY_COMPLEMENT_PLUS_LUT };

  for (size_t n = 0; n < num_images; ++n)
  {
    nlt_setting baseline;
    baseline.type = param_nlt::OJPH_NLT_NO_NLT;
    baseline.use_pfm_lut = true;
    baseline.reversible = false;
    baseline.qstep = default_qstep();
    comparison plain = round_trip(std::string("nlt_plain_") + image_names[n],
      images[n], baseline);

    for (size_t t = 0; t < 3; ++t)
    {
      nlt_setting setting = baseline;
      setting.type = types[t];
      const std::string tag = std::string("nlt_") +
        std::to_string((int)types[t]) + "_" + image_names[n];
      comparison nlt = round_trip(tag, images[n], setting);

      std::cout << image_names[n] << " with nonlinearity " << (int)types[t]
        << ": mean absolute error " << nlt.mean_abs_error
        << ", mean relative error " << nlt.mean_rel_error
        << ", largest relative error " << nlt.max_rel_error
        << " (without a nonlinearity the mean absolute error is "
        << plain.mean_abs_error << ")" << std::endl;

      // type 3 is a reversible transformation of the samples, so it has to
      // behave much like no nonlinearity at all
      const double factor =
        types[t] == param_nlt::OJPH_NLT_BINARY_COMPLEMENT_NLT ? 2.0 : 5.0;

      EXPECT_LE(nlt.mean_abs_error, factor * plain.mean_abs_error + 1024.0)
        << image_names[n] << " with nonlinearity " << (int)types[t]
        << ": mean absolute error " << nlt.mean_abs_error << " against "
        << plain.mean_abs_error << " for a round trip without a nonlinearity";
      EXPECT_LT(nlt.mean_rel_error, 0.05)
        << image_names[n] << " with nonlinearity " << (int)types[t];
      EXPECT_LT(nlt.max_rel_error, 0.30)
        << image_names[n] << " with nonlinearity " << (int)types[t];
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Writing a codestream that combines a LUT style nonlinearity with the
// reversible wavelet has to fail: the nonlinearity cannot be applied with that
// wavelet, so the codestream would advertise a points transformation that its
// samples do not follow.
///////////////////////////////////////////////////////////////////////////////
TEST(NltTest, LutStyleIsRejectedWithReversibleWavelet)
{
  test_image img;
  make_synthetic_image(img, 64, 64);

  const ui8 types[2] = { param_nlt::OJPH_NLT_LUT_STYLE_NLT,
                         param_nlt::OJPH_NLT_BINARY_COMPLEMENT_PLUS_LUT };
  for (size_t i = 0; i < 2; ++i)
  {
    nlt_setting setting;
    setting.type = types[i];
    setting.use_pfm_lut = true;
    setting.reversible = true;             // the combination that is refused
    setting.qstep = default_qstep();

    const std::string filename = OUT_FILE_DIR "nlt_rejected.j2c";
    message_capture capture;
    EXPECT_THROW(encode_image(filename, img, setting), std::runtime_error)
      << "nonlinearity " << (int)types[i] << " with the reversible wavelet";
    EXPECT_EQ(capture.errors.last_code, 0x000501B1)
      << "nonlinearity " << (int)types[i];
  }

  // the same nonlinearity is accepted with the irreversible wavelet, and the
  // binary complement nonlinearity is accepted with either wavelet; those
  // combinations are covered by RoundTripOfTestImages and
  // BinaryComplementIsLossless
  nlt_setting accepted;
  accepted.type = param_nlt::OJPH_NLT_LUT_STYLE_NLT;
  accepted.use_pfm_lut = true;
  accepted.reversible = false;
  accepted.qstep = default_qstep();
  EXPECT_NO_THROW(encode_image(OUT_FILE_DIR "nlt_accepted.j2c", img, accepted));
}

///////////////////////////////////////////////////////////////////////////////
// The wavelet can be chosen per component, with a COC marker segment, so the
// refusal has to follow the component the nonlinearity is attached to.
///////////////////////////////////////////////////////////////////////////////
TEST(NltTest, LutStyleIsRejectedPerComponent)
{
  test_image img;
  make_synthetic_image(img, 64, 64);

  // component 1 is coded with the irreversible wavelet, so the nonlinearity is
  // accepted there
  {
    const std::string filename = OUT_FILE_DIR "nlt_component_accepted.j2c";
    EXPECT_NO_THROW(encode_two_component_image(filename, img, 1));
    test_image decoded;
    decode_image(filename, decoded, NULL, NULL, NULL, NULL);
    EXPECT_EQ(decoded.num_comps, 2u);
  }

  // component 0 keeps the reversible wavelet, so the nonlinearity is refused
  // there
  {
    const std::string filename = OUT_FILE_DIR "nlt_component_rejected.j2c";
    message_capture capture;
    EXPECT_THROW(encode_two_component_image(filename, img, 0),
      std::runtime_error);
    EXPECT_EQ(capture.errors.last_code, 0x000501B1);
  }
}

///////////////////////////////////////////////////////////////////////////////
// A codestream that carries the combination, which another encoder may have
// produced, has to be reported when its headers are read: this library cannot
// invert such a nonlinearity, so it says so rather than return samples that
// look like image samples.
///////////////////////////////////////////////////////////////////////////////
TEST(NltTest, LutStyleWithReversibleWaveletIsReportedWhenRead)
{
  test_image img;
  make_synthetic_image(img, 64, 64);

  // a reversible codestream without a nonlinearity, into which an NLT marker
  // segment for a LUT style nonlinearity is inserted
  nlt_setting setting;
  setting.type = param_nlt::OJPH_NLT_NO_NLT;
  setting.use_pfm_lut = true;
  setting.reversible = true;
  setting.qstep = default_qstep();

  const std::string filename = OUT_FILE_DIR "nlt_foreign.j2c";
  encode_image(filename, img, setting);
  ASSERT_TRUE(insert_lut_nlt_marker(filename,
    param_nlt::OJPH_NLT_LUT_STYLE_NLT));

  message_capture capture;
  codestream cs;
  j2c_infile file;
  file.open(filename.c_str());
  EXPECT_THROW(cs.read_headers(&file), std::runtime_error);
  EXPECT_GE(capture.errors.num_messages, 1);
  EXPECT_EQ(capture.errors.last_code, 0x00030059);

  ui8 bit_depth = 0, type = 0;
  bool is_signed = false;
  EXPECT_TRUE(cs.access_nlt().get_nonlinear_transform(param_nlt::ALL_COMPS,
    bit_depth, is_signed, type));
  EXPECT_EQ((int)type, (int)param_nlt::OJPH_NLT_LUT_STYLE_NLT);
  cs.close();
}
