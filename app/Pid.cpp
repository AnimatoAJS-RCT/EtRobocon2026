#include "Pid.h"
#include "Log.h"
#include <cstdio>
#include <numeric>
#include <unistd.h>
#include <string.h>
#include <math.h>

PidGain::PidGain(double _kp, double _ki, double _kd) : kp(_kp), ki(_ki), kd(_kd) {}

Pid::Pid(PidGain *_pidGain, unsigned int _differenceRange)
  : gain(_pidGain), integral(0.0), integralLimit(0.0), differenceRange(_differenceRange)
{
  //// カレントディレクトリ取得
  // getcwd(pathname, PATHNAME_SIZE);
  // strcat(pathname, "/pid.csv");
  // LOGD("現在のファイルパス:%s\n", pathname);
  // FILE *file;//ファイルポインタを宣言
  // file=fopen(pathname,"a");//ファイルをオープン(名前の指定)
  // fprintf(file,"init\n");//書き込み
  // fclose(file);//ファイルを閉じる
}

void Pid::reset()
{
  integral = 0.0;
  pastDeviations.clear();
}

void Pid::setIntegralLimit(double limit)
{
  integralLimit = limit;
}

double Pid::calculatePid(double diff, double delta)
{
  // 0除算を避けるためにdelta=0の場合はデフォルト周期0.01とする
  if (delta == 0)
    delta = 0.01;
  // 現在の偏差を求める
  double currentDeviation = diff;
  // 積分の処理を行う
  integral += currentDeviation * delta;
  if(integralLimit > 0.0) {
    if(integral > integralLimit) integral = integralLimit;
    if(integral < -integralLimit) integral = -integralLimit;
  }
  // 微分の処理を行う
  // 過去偏差数を超過する偏差の履歴を消す
  while (pastDeviations.size() >= differenceRange)
  {
    pastDeviations.erase(pastDeviations.begin());
  }
  // 今回の偏差を履歴に追加
  pastDeviations.push_back(currentDeviation);
  // 偏差の履歴をもとに微分
  double difference;
  if (pastDeviations.size() < differenceRange)
  {
    // 偏差の履歴の数が微分するのに足りない場合：D制御は0とする
    difference = 0;
  }
  else
  {
    // 偏差の履歴の数が微分するのに足りる場合：微分する
    double sumX, sumY, sumProd;
    sumX = sumY = sumProd = 0.0;
    for (unsigned int i = 0; i < differenceRange; i++)
    {
      sumX += (double)(i + 1);
      sumY += pastDeviations[i];
      sumProd += (double)(i + 1) * pastDeviations[i];
    }
    double aveX = sumX / (double)differenceRange;
    double aveY = sumY / (double)differenceRange;
    double cov = sumProd / (double)differenceRange - aveX * aveY; // 共分散
    double varX = 0;                                              // 分散X
    for (unsigned int i = 0; i < differenceRange; i++)
    {
      varX += pow((double)(i + 1) - aveX, 2);
    }
    varX = varX / (double)differenceRange;
    if (varX == 0)
    {
      difference = 0;
    }
    else
    {
      difference = cov / varX / delta;
    }
    // LOGD("%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n", sumX, sumY, sumProd, aveX, aveY, cov, varX, difference);
  }

  // P制御の計算を行う
  double p = gain->kp * currentDeviation;
  // I制御の計算を行う
  double i = gain->ki * integral;
  // D制御の計算を行う
  double d = gain->kd * difference;

  // デバッグ用
  LOGD_EVERY(200, "[PID]\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n", diff,
             currentDeviation, integral, difference, p, i, d, (p + i + d));
  // LOGD("CV: %lf, PID: %lf\n", currentValue, (p + i + d));
  //  FIXME:ファイル出力すると書き込みに時間がかかりすぎて走行結果に大きく影響する（書き込みの間、設定したPWMで車輪が動き続けるため）
  // FILE *file;//ファイルポインタを宣言
  // file=fopen(pathname,"a");//ファイルをオープン(名前の指定)
  // fprintf(file,"%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n", targetValue,currentValue,currentDeviation,integral,difference,p,i,d,(p+i+d));//書き込み
  // fclose(file);//ファイルを閉じる

  // 操作量 = P制御 + I制御 + D制御
  return (p + i + d);
}