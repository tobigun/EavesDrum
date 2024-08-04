#include "drum/sensing/scale.h"

#include <unity.h>

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void test_scale_fullRange() {
  const sensor_value_t inMin = 0;
  const sensor_value_t inMax = 1023;
  TEST_ASSERT_EQUAL_UINT16(0, scale(0, inMin, inMax));
  TEST_ASSERT_EQUAL_UINT16(1, scale(1, inMin, inMax));
  TEST_ASSERT_EQUAL_UINT16(512, scale(512, inMin, inMax));
  TEST_ASSERT_EQUAL_UINT16(1023, scale(1023, inMin, inMax));
}

void test_scale_minMaxRange() {
  const sensor_value_t inMin = 100;
  const sensor_value_t inMax = 500;
  TEST_ASSERT_EQUAL_UINT16(0, scale(0, inMin, inMax));
  TEST_ASSERT_EQUAL_UINT16(0, scale(1, inMin, inMax));
  TEST_ASSERT_EQUAL_UINT16(0, scale(99, inMin, inMax));
  TEST_ASSERT_EQUAL_UINT16(1, scale(100, inMin, inMax));
  TEST_ASSERT_EQUAL_UINT16(512, scale(300, inMin, inMax));
  TEST_ASSERT_EQUAL_UINT16(1023, scale(500, inMin, inMax));
  TEST_ASSERT_EQUAL_UINT16(1023, scale(1023, inMin, inMax));
}

void test_scale_fullSmallRange() {
  const sensor_value_t inMin = 0;
  const sensor_value_t inMax = 1;
  TEST_ASSERT_EQUAL_UINT16(0, scale(0, inMin, inMax));
  TEST_ASSERT_EQUAL_UINT16(1023, scale(1, inMin, inMax));
  TEST_ASSERT_EQUAL_UINT16(1023, scale(512, inMin, inMax));
  TEST_ASSERT_EQUAL_UINT16(1023, scale(1023, inMin, inMax));
}

// should be blocked by the UI, but make sure it does not crash in case it occurs
void test_scale_noRange() {
  const sensor_value_t inMin = 0;
  const sensor_value_t inMax = 0;
  TEST_ASSERT_EQUAL_UINT16(0, scale(0, inMin, inMax));
  TEST_ASSERT_EQUAL_UINT16(1023, scale(1, inMin, inMax));
  TEST_ASSERT_EQUAL_UINT16(1023, scale(512, inMin, inMax));
  TEST_ASSERT_EQUAL_UINT16(1023, scale(1023, inMin, inMax));
}

void test_curve_linear() {
  const sensor_value_t inMin = 100;
  const sensor_value_t inMax = 500;
  const CurveType curveType = CurveType::Linear;
  TEST_ASSERT_EQUAL_UINT16(0, scaleAndCurve(0, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(0, scaleAndCurve(1, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(0, scaleAndCurve(99, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(1, scaleAndCurve(100, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(1, scaleAndCurve(101, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(64, scaleAndCurve(300, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(127, scaleAndCurve(500, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(127, scaleAndCurve(1023, inMin, inMax, curveType));
}

void test_curve_log1() {
  const sensor_value_t inMin = 100;
  const sensor_value_t inMax = 500;
  const CurveType curveType = CurveType::Logarithmic1;
  TEST_ASSERT_EQUAL_UINT16(0, scaleAndCurve(0, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(0, scaleAndCurve(1, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(0, scaleAndCurve(99, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(1, scaleAndCurve(100, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(2, scaleAndCurve(101, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(94, scaleAndCurve(300, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(127, scaleAndCurve(500, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(127, scaleAndCurve(1023, inMin, inMax, curveType));
}

void test_curve_log2() {
  const sensor_value_t inMin = 100;
  const sensor_value_t inMax = 500;
  const CurveType curveType = CurveType::Logarithmic2;
  TEST_ASSERT_EQUAL_UINT16(0, scaleAndCurve(0, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(0, scaleAndCurve(1, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(0, scaleAndCurve(99, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(1, scaleAndCurve(100, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(2, scaleAndCurve(101, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(113, scaleAndCurve(300, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(127, scaleAndCurve(500, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(127, scaleAndCurve(1023, inMin, inMax, curveType));
}

void test_curve_exp1() {
  const sensor_value_t inMin = 100;
  const sensor_value_t inMax = 500;
  const CurveType curveType = CurveType::Exponential1;
  TEST_ASSERT_EQUAL_UINT16(0, scaleAndCurve(0, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(0, scaleAndCurve(1, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(0, scaleAndCurve(99, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(1, scaleAndCurve(100, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(1, scaleAndCurve(101, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(3, scaleAndCurve(120, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(13, scaleAndCurve(200, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(34, scaleAndCurve(300, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(69, scaleAndCurve(400, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(126, scaleAndCurve(499, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(127, scaleAndCurve(500, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(127, scaleAndCurve(1023, inMin, inMax, curveType));
}

void test_curve_exp2() {
  const sensor_value_t inMin = 100;
  const sensor_value_t inMax = 500;
  const CurveType curveType = CurveType::Exponential2;

  TEST_ASSERT_EQUAL_UINT16(0, scaleAndCurve(0, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(0, scaleAndCurve(1, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(0, scaleAndCurve(99, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(1, scaleAndCurve(100, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(1, scaleAndCurve(101, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(1, scaleAndCurve(120, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(5, scaleAndCurve(200, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(15, scaleAndCurve(300, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(45, scaleAndCurve(400, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(125, scaleAndCurve(499, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(127, scaleAndCurve(500, inMin, inMax, curveType));
  TEST_ASSERT_EQUAL_UINT16(127, scaleAndCurve(1023, inMin, inMax, curveType));
}

void printCurveFunc() {
#if 0 // enable this to print out a curve to a file
  const CurveType curveType = CurveType::Exponential2;
  const sensor_value_t inMin = 100;
  const sensor_value_t inMax = 500;

  FILE* f = fopen("curve.txt", "w");
  for (int i = 0; i < MAX_SENSOR_VALUE; ++i) {
    int value = scaleAndCurve(i, inMin, inMax, curveType);
    fprintf(f, "%d: %d\n", i, value);
  }
  fclose(f);
#endif
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(printCurveFunc);
  RUN_TEST(test_scale_fullRange);
  RUN_TEST(test_scale_minMaxRange);
  RUN_TEST(test_scale_fullSmallRange);
  RUN_TEST(test_scale_noRange);
  RUN_TEST(test_curve_linear);
  RUN_TEST(test_curve_log1);
  RUN_TEST(test_curve_log2);
  RUN_TEST(test_curve_exp1);
  RUN_TEST(test_curve_exp2);
  return UNITY_END();
}
