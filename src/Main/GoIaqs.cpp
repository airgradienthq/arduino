#include "GoIaqs.h"

#include <math.h>

namespace {

struct Anchor {
  float x;
  int y;
};

/** White-paper PM2.5 anchors (ug/m^3, score). */
const Anchor PM25_ANCHORS[] = {
    {0.0f, 10}, {10.0f, 8}, {11.0f, 7}, {25.0f, 4}, {26.0f, 3}, {100.0f, 0},
};

/** White-paper CO2 anchors (ppm, score). */
const Anchor CO2_ANCHORS[] = {
    {400.0f, 10}, {800.0f, 8},  {801.0f, 7},
    {1400.0f, 4}, {1401.0f, 3}, {5000.0f, 0},
};

const float PM25_MIN = 0.0f;
const float PM25_MAX = 100.0f;
const float CO2_MIN = 400.0f;
const float CO2_MAX = 5000.0f;

inline float clampF(float v, float lo, float hi) {
  if (v < lo) {
    return lo;
  }
  if (v > hi) {
    return hi;
  }
  return v;
}

inline int clampI(int v, int lo, int hi) {
  if (v < lo) {
    return lo;
  }
  if (v > hi) {
    return hi;
  }
  return v;
}

/** Round-half-up (matching the reference TS implementation). */
inline int roundHalfUp(float v) { return (int)floorf(v + 0.5f); }

int interpolate(float value, const Anchor *anchors, size_t count) {
  if (value <= anchors[0].x) {
    return anchors[0].y;
  }
  const Anchor &last = anchors[count - 1];
  if (value >= last.x) {
    return last.y;
  }
  for (size_t i = 0; i < count - 1; i++) {
    const Anchor &left = anchors[i];
    const Anchor &right = anchors[i + 1];
    if (value <= right.x) {
      float interp =
          (float)left.y +
          ((value - left.x) * (float)(right.y - left.y)) / (right.x - left.x);
      return clampI(roundHalfUp(interp), GoIaqs::SCORE_MIN, GoIaqs::SCORE_MAX);
    }
  }
  return GoIaqs::SCORE_MIN;
}

int pollutantScore(float value, float min, float max, const Anchor *anchors,
                   size_t count) {
  if (!isfinite(value) || value > max) {
    return GoIaqs::SCORE_MIN;
  }
  return interpolate(clampF(value, min, max), anchors, count);
}

} // namespace

int GoIaqs::pm25Score(float pm25UgM3) {
  return pollutantScore(pm25UgM3, PM25_MIN, PM25_MAX, PM25_ANCHORS,
                        sizeof(PM25_ANCHORS) / sizeof(PM25_ANCHORS[0]));
}

int GoIaqs::co2Score(float co2Ppm) {
  return pollutantScore(co2Ppm, CO2_MIN, CO2_MAX, CO2_ANCHORS,
                        sizeof(CO2_ANCHORS) / sizeof(CO2_ANCHORS[0]));
}

int GoIaqs::totalScore(int pm25, int co2) {
  if (pm25 == co2) {
    if (pm25 <= 7) {
      int reduced = pm25 - 1;
      return reduced < SCORE_MIN ? SCORE_MIN : reduced;
    }
    return pm25;
  }
  return pm25 < co2 ? pm25 : co2;
}

GoIaqs::Category GoIaqs::categoryOf(int totalScore) {
  if (totalScore >= 8) {
    return CategoryGood;
  }
  if (totalScore >= 4) {
    return CategoryModerate;
  }
  return CategoryUnhealthy;
}

GoIaqs::Rgb GoIaqs::colorOf(Category category) {
  switch (category) {
  case CategoryGood:
    return Rgb{0x64, 0x8E, 0xFF};
  case CategoryModerate:
    return Rgb{255, 128, 0};
  case CategoryUnhealthy:
  default:
    return Rgb{255, 0, 0};
  }
}

GoIaqs::Dominant GoIaqs::dominantOf(int pm25Score, int co2Score) {
  if (pm25Score == co2Score) {
    return DominantBoth;
  }
  return (pm25Score < co2Score) ? DominantPm25 : DominantCo2;
}

char GoIaqs::letterOf(Category category) {
  switch (category) {
  case CategoryGood:
    return 'A';
  case CategoryModerate:
    return 'B';
  case CategoryUnhealthy:
  default:
    return 'Z';
  }
}
