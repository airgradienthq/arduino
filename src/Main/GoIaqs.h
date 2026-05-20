#ifndef _GO_IAQS_H_
#define _GO_IAQS_H_

#include <stdint.h>

/**
 * @brief GO IAQS Starter Score implementation.
 *
 * Reference:
 * https://www.airgradient.com/blog/go-iaqs-starter-score-technical-implementation
 * Attribution: Achim Haug (AirGradient) for GO AQS. License: CC BY-SA 4.0.
 *
 * Converts PM2.5 (ug/m^3) and CO2 (ppm) concentrations into an integer
 * "0..10" total score plus category and category color, using piecewise
 * linear anchors and the synergetic combination rule defined in the
 * white paper.
 *
 * Pure computation; no Arduino / hardware dependencies.
 */
class GoIaqs {
public:
  enum Category {
    CategoryGood = 0,      /** total score 8..10, color #648EFF */
    CategoryModerate = 1,  /** total score 4..7,  color #FFB000 */
    CategoryUnhealthy = 2, /** total score 0..3,  color #FF190C */
  };

  enum Dominant {
    DominantPm25 = 0, /** PM2.5 has the worse per-pollutant score */
    DominantCo2 = 1,  /** CO2 has the worse per-pollutant score */
    DominantBoth = 2, /** PM2.5 and CO2 per-pollutant scores are equal */
  };

  struct Rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
  };

  /** Score bounds (inclusive). */
  static const int SCORE_MIN = 0;
  static const int SCORE_MAX = 10;

  /**
   * @brief Compute per-pollutant score for PM2.5.
   *
   * @param pm25UgM3 PM2.5 concentration in ug/m^3.
   * @return int in [SCORE_MIN .. SCORE_MAX]
   */
  static int pm25Score(float pm25UgM3);

  /**
   * @brief Compute per-pollutant score for CO2.
   *
   * @param co2Ppm CO2 concentration in ppm.
   * @return int in [SCORE_MIN .. SCORE_MAX]
   */
  static int co2Score(float co2Ppm);

  /**
   * @brief Combine two per-pollutant scores into a single total score
   * applying the synergetic rule:
   *  - If scores differ: total = min(pm25, co2).
   *  - If equal and shared <= 7: total = max(shared - 1, 0).
   *  - If equal and shared >= 8: total = shared.
   */
  static int totalScore(int pm25, int co2);

  /**
   * @brief Map a total score to a category (Good / Moderate / Unhealthy).
   */
  static Category categoryOf(int totalScore);

  /**
   * @brief Get the RGB color associated with a category, tuned for the
   * LED bar (may differ from the white-paper hex values to match the
   * physical LED output).
   */
  static Rgb colorOf(Category category);

  /**
   * @brief Identify the pollutant that drives the total score. The lower
   * per-pollutant score wins; equal scores return DominantBoth.
   */
  static Dominant dominantOf(int pm25Score, int co2Score);

  /**
   * @brief Get the single-character letter grade for a category, per the
   * white paper: Good -> 'A', Moderate -> 'B', Unhealthy -> 'Z'.
   */
  static char letterOf(Category category);
};

#endif /** _GO_IAQS_H_ */
