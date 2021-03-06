/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "hll.hpp"

#include <catch.hpp>

namespace datasketches {

static void runCheckCopy(int lgConfigK, target_hll_type tgtHllType) {
  hll_sketch sk(lgConfigK, tgtHllType);

  for (int i = 0; i < 7; ++i) {
    sk.update(i);
  }

  hll_sketch skCopy = sk;
  REQUIRE(sk.get_estimate() == skCopy.get_estimate());

  // no access to hllSketchImpl, so we'll ensure those differ by adding more
  // data to sk and ensuring the mode and estimates differ
  for (int i = 7; i < 24; ++i) {
    sk.update(i);
  }
  REQUIRE(16.0 < (sk.get_estimate() - skCopy.get_estimate()));

  skCopy = sk;
  REQUIRE(sk.get_estimate() == skCopy.get_estimate());

  int u = (sk.get_target_type() == HLL_4) ? 100000 : 25;
  for (int i = 24; i < u; ++i) {
    sk.update(i);
  }
  REQUIRE(sk.get_estimate() != skCopy.get_estimate()); // either 1 or 100k difference

  skCopy = sk;
  REQUIRE(sk.get_estimate() == skCopy.get_estimate());
}

TEST_CASE("hll sketch: check copies", "[hll_sketch]") {
  runCheckCopy(14, HLL_4);
  runCheckCopy(8, HLL_6);
  runCheckCopy(8, HLL_8);
}

static void copyAs(target_hll_type srcType, target_hll_type dstType) {
  int lgK = 8;
  int n1 = 7;
  int n2 = 24;
  int n3 = 1000;
  int base = 0;

  hll_sketch src(lgK, srcType);
  for (int i = 0; i < n1; ++i) {
    src.update(i + base);
  }
  hll_sketch dst(src, dstType);
  REQUIRE(src.get_estimate() == dst.get_estimate());

  for (int i = n1; i < n2; ++i) {
    src.update(i + base);
  }
  dst = hll_sketch(src, dstType);
  REQUIRE(src.get_estimate() == dst.get_estimate());

  for (int i = n2; i < n3; ++i) {
    src.update(i + base);
  }
  dst = hll_sketch(src, dstType);
  REQUIRE(src.get_estimate() == dst.get_estimate());
}

TEST_CASE("hll sketch: check copy as", "[hll_sketch]") {
  copyAs(HLL_4, HLL_4);
  copyAs(HLL_4, HLL_6);
  copyAs(HLL_4, HLL_8);
  copyAs(HLL_6, HLL_4);
  copyAs(HLL_6, HLL_6);
  copyAs(HLL_6, HLL_8);
  copyAs(HLL_8, HLL_4);
  copyAs(HLL_8, HLL_6);
  copyAs(HLL_8, HLL_8);
}

TEST_CASE("hll sketch: check misc1", "[hll_sketch]") {
  int lgConfigK = 8;
  target_hll_type srcType = target_hll_type::HLL_8;
  hll_sketch sk(lgConfigK, srcType);

  for (int i = 0; i < 7; ++i) { sk.update(i); } // LIST
  REQUIRE(sk.get_compact_serialization_bytes() == 36);
  REQUIRE(sk.get_updatable_serialization_bytes() == 40);

  for (int i = 7; i < 24; ++i) { sk.update(i); } // SET
  REQUIRE(sk.get_compact_serialization_bytes() == 108);
  REQUIRE(sk.get_updatable_serialization_bytes() == 140);

  sk.update(24); // HLL
  REQUIRE(sk.get_updatable_serialization_bytes() == 40 + 256);

  const int hllBytes = HllUtil<>::HLL_BYTE_ARR_START + (1 << lgConfigK);
  REQUIRE(sk.get_compact_serialization_bytes() == hllBytes);
  REQUIRE(hll_sketch::get_max_updatable_serialization_bytes(lgConfigK, HLL_8) == hllBytes);
}

TEST_CASE("hll sketch: check num std dev", "[hll_sketch]") {
  REQUIRE_THROWS_AS(HllUtil<>::checkNumStdDev(0), std::invalid_argument);
}

void checkSerializationSizes(const int lgConfigK, target_hll_type tgtHllType) {
  hll_sketch sk(lgConfigK, tgtHllType);
  int i;

  // LIST
  for (i = 0; i < 7; ++i) { sk.update(i); }
  int expected = HllUtil<>::LIST_INT_ARR_START + (i << 2);
  REQUIRE(sk.get_compact_serialization_bytes() == expected);
  expected = HllUtil<>::LIST_INT_ARR_START + (4 << HllUtil<>::LG_INIT_LIST_SIZE);
  REQUIRE(sk.get_updatable_serialization_bytes() == expected);

  // SET
  for (i = 7; i < 24; ++i) { sk.update(i); }
  expected = HllUtil<>::HASH_SET_INT_ARR_START + (i << 2);
  REQUIRE(sk.get_compact_serialization_bytes() == expected);
  expected = HllUtil<>::HASH_SET_INT_ARR_START + (4 << HllUtil<>::LG_INIT_SET_SIZE);
  REQUIRE(sk.get_updatable_serialization_bytes() == expected);
}

TEST_CASE("hll sketch: check ser sizes", "[hll_sketch]") {
  checkSerializationSizes(8, HLL_8);
  checkSerializationSizes(8, HLL_6);
  checkSerializationSizes(8, HLL_4);
}

TEST_CASE("hll sketch: exercise to string", "[hll_sketch]") {
  hll_sketch sk(15, HLL_4);
  for (int i = 0; i < 25; ++i) { sk.update(i); }
  std::ostringstream oss(std::ios::binary);
  sk.to_string(oss, false, true, true, true);
  for (int i = 25; i < (1 << 20); ++i) { sk.update(i); }
  sk.to_string(oss, false, true, true, true);
  sk.to_string(oss, false, true, true, false);

  sk = hll_sketch(8, HLL_8);
  for (int i = 0; i < 25; ++i) { sk.update(i); }
  sk.to_string(oss, false, true, true, true);
}

// Creates and serializes then deserializes sketch.
// Returns true if deserialized sketch is compact.
static bool checkCompact(const int lgK, const int n, const target_hll_type type, bool compact) {
  hll_sketch sk(lgK, type);
  for (int i = 0; i < n; ++i) { sk.update(i); }
  
  std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
  if (compact) { 
    sk.serialize_compact(ss);
    REQUIRE(ss.tellp() == sk.get_compact_serialization_bytes());
  } else {
    sk.serialize_updatable(ss);
    REQUIRE(ss.tellp() == sk.get_updatable_serialization_bytes());
  }
  
  hll_sketch sk2 = hll_sketch::deserialize(ss);
  REQUIRE(sk2.get_estimate() == Approx(n).margin(0.01));
  bool isCompact = sk2.is_compact();

  return isCompact;
}

TEST_CASE("hll sketch: check compact flag", "[hll_sketch]") {
  int lgK = 8;
  // unless/until we create non-updatable "direct" versions,
  // deserialized image should never be compact
  // LIST: follows serialization request
  REQUIRE(checkCompact(lgK, 7, HLL_8, false) == false);
  REQUIRE(checkCompact(lgK, 7, HLL_8, true) == false);

  // SET: follows serialization request
  REQUIRE(checkCompact(lgK, 24, HLL_8, false) == false);
  REQUIRE(checkCompact(lgK, 24, HLL_8, true) == false);

  // HLL8: always updatable
  REQUIRE(checkCompact(lgK, 25, HLL_8, false) == false);
  REQUIRE(checkCompact(lgK, 25, HLL_8, true) == false);

  // HLL6: always updatable
  REQUIRE(checkCompact(lgK, 25, HLL_6, false) == false);
  REQUIRE(checkCompact(lgK, 25, HLL_6, true) == false);

  // HLL4: follows serialization request
  REQUIRE(checkCompact(lgK, 25, HLL_4, false) == false);
  REQUIRE(checkCompact(lgK, 25, HLL_4, true) == false);
}

TEST_CASE("hll sketch: check k limits", "[hll_sketch]") {
  hll_sketch sketch1(HllUtil<>::MIN_LOG_K, target_hll_type::HLL_8);
  hll_sketch sketch2(HllUtil<>::MAX_LOG_K, target_hll_type::HLL_4);
  REQUIRE_THROWS_AS(hll_sketch(HllUtil<>::MIN_LOG_K - 1), std::invalid_argument);

  REQUIRE_THROWS_AS(hll_sketch(HllUtil<>::MAX_LOG_K + 1), std::invalid_argument);
}

TEST_CASE("hll sketch: check input types", "[hll_sketch]") {
  hll_sketch sk(8, target_hll_type::HLL_8);

  // inserting the same value as a variety of input types
  sk.update((uint8_t) 102);
  sk.update((uint16_t) 102);
  sk.update((uint32_t) 102);
  sk.update((uint64_t) 102);
  sk.update((int8_t) 102);
  sk.update((int16_t) 102);
  sk.update((int32_t) 102);
  sk.update((int64_t) 102);
  REQUIRE(sk.get_estimate() == Approx(1.0).margin(0.01));

  // identical binary representations
  // no unsigned in Java, but need to sign-extend both as Java would do 
  sk.update((uint8_t) 255);
  sk.update((int8_t) -1);

  sk.update((float) -2.0);
  sk.update((double) -2.0);

  std::string str = "input string";
  sk.update(str);
  sk.update(str.c_str(), str.length());
  REQUIRE(sk.get_estimate() == Approx(4.0).margin(0.01));

  sk = hll_sketch(8, target_hll_type::HLL_6);
  sk.update((float) 0.0);
  sk.update((float) -0.0);
  sk.update((double) 0.0);
  sk.update((double) -0.0);
  REQUIRE(sk.get_estimate() == Approx(1.0).margin(0.01));

  sk = hll_sketch(8, target_hll_type::HLL_4);
  sk.update(std::nanf("3"));
  sk.update(std::nan("9"));
  REQUIRE(sk.get_estimate() == Approx(1.0).margin(0.01));

  sk = hll_sketch(8, target_hll_type::HLL_4);
  sk.update(nullptr, 0);
  sk.update("");
  REQUIRE(sk.is_empty());
}



} /* namespace datasketches */
