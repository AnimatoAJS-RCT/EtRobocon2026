#ifndef PID_H
#define PID_H
#define PATHNAME_SIZE 512

#include <vector>

// PIDゲインを保持する構造体
struct PidGain
{
public:
  double kp; // Pゲイン
  double ki; // Iゲイン
  double kd; // Dゲイン

  /** コンストラクタ
   * @param _kp Pゲイン
   * @param _ki Iゲイン
   * @param _kd Dゲイン
   */
  PidGain(double _kp, double _ki, double _kd);
};

class Pid
{
public:
  /** コンストラクタ
   * @param _kp Pゲイン
   * @param _ki Iゲイン
   * @param _kd Dゲイン
   * @param _differenceRange 微分に使用する過去の偏差の数 1~(理想は1でよいが、EV3RTのセンサー値が短時間では更新されない場合があるため複数の過去の値で微分できるようにする)
   */
  Pid(PidGain *_pidGain, unsigned int _differenceRange = 1);

  /**
   * @fn double calculatePid(double currentValue, double delta = 0.01);
   * @brief PIDを計算する
   * @param currentValue 現在値
   * @param delta 周期[ms](デフォルト値0.01[10ms]、省略可)
   * @return PIDの計算結果(操作量)
   */
  double calculatePid(double currentValue, double delta = 0.01);
  void reset();
  void setIntegralLimit(double limit);

private:
  PidGain *gain;
  std::vector<double> pastDeviations; // 過去の偏差
  double integral;                    // 偏差の累積
  double integralLimit;               // 偏差累積の絶対値上限。0以下は無制限
  unsigned int differenceRange;       // 微分に使用する過去の偏差の数
  char pathname[PATHNAME_SIZE];  // ファイルパス
};

#endif