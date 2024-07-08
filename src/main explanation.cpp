#include <Arduino.h>
#include <M5Unified.h>
#include "arduinoFFT.h"

// ref:
// https://ambidata.io/samples/m5stack/sound/

#define MIC 33 // for Core2's PortA (Pin1)
	/*
	コード中の MIC という単語を、コンパイル時にすべて 33 に置き換えるという意味
	33はMICがつながれているピン番号を指す。
	#define：定数(≠変数)やマクロを定義する。
	*/
//#define SAMPLING_FREQUENCY 40000 // 40kHz
//#define SAMPLING_FREQUENCY 2000 // 2kHz
#define SAMPLING_FREQUENCY 4000 // 4kHz
	/*
	サンプリング周波数を定める。
	サンプリング周波数：サンプルを何秒ごとにとるか。サンプルをとってから次のサンプルをとるまでの時間。音の周波数とは別物。
	サンプリング定理：サンプリング周波数をfsとおくと、fs/2以上の周波数の情報はとれない。
		参考：https://qiita.com/panda11/items/e28ae434c0dd64a2dbb7 
	*/
const uint16_t FFTsamples = 256;  // サンプル数は2のべき乗
	/*
	一回でフーリエ変換するサンプル数を定める。
	const：定数の宣言をする修飾子。この変数(FFTsamples)の値が一度設定された後、変更されないことを意味する。
	uint15_t：16ビットの符号なし整数。範囲は0~65535
	*/

double vReal[FFTsamples];  // vReal[]にサンプリングしたデーターを入れる
double vImag[FFTsamples];
	/*
	フーリエ変換で使う複素数データの実部(vReal)と虚部(vImag)の宣言。
	vRealはまずサンプリングデータが格納され、FFTの計算が終わったらフーリエ変換による複素数の実部が格納され、最後に複素数を実数に変換したものが格納されて出力される。
	*/
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, FFTsamples, SAMPLING_FREQUENCY);  // FFTオブジェクトを作る
	/*
	ArduinoFFTライブラリのオブジェクトを作成し、(実部データ, 虚部データ, サンプル数, サンプリング周波数)を指定して初期化する。
	ArduinoFFT：FFT(高速フーリエ変換)を行うためのクラス。
	FFT：ArduinoFFTのインスタンスの名称。
	*/

unsigned int sampling_period_us;
	/*
	unsigned：変数(sampling_period_us)が非負の整数値のみ扱うことを示す修飾子。範囲の大きさは変わらない。例えば範囲が-n~nである整数型につけると範囲が0~2nになる。
	*/

void sample(int nsamples) {
	/*
	nsamples：sample関数を呼び出すときの引数。loop関数でsample関数を呼び出すときはFFTsamples=256とおかれている。最初から引数をFFTsamplesとおいてもいい。
	*/
	for (int i = 0; i < nsamples; i++) {
		unsigned long t = micros();
			/*
			micros()：現在の時刻をマイクロ秒単位で取得する。
			*/
		vReal[i] = (double)analogRead(MIC) / 4095.0 * 3.6 + 0.1132; // ESP32のADCの特性を補正
		vImag[i] = 0;
			/*
			vRealにMICから読み取ったアナログ値を格納し、vImagの初期値を0にする。
			*/
		while ((micros() - t) < sampling_period_us) ;
			/*
			whileループは()内の条件が真である間ブロック内の文を繰り返し実行するが、この場合ループの中身が空文であるから条件が満たされるまで待機するという意味になる。
			*/
  }
}

int X0 = 30;
int Y0 = 20;
int _height = 240 - Y0;
int _width = 320;
float dmax = 5.0;
	/*
	X0：描画するグラフの左端のx座標
	Y0：画面の下端から描画するグラフの下端までの長さ
	Question1.M5Stackの画面の座標(0,0)は左上なのになんでY0が下端になる？
	_height：
	Question2(1).240はどこから出てきた？何を表してる？
	_width：グラフの幅
	dmaxが大きくなると入力値(音量)が同じでも出力されるグラフの高さは相対的に低くなる。
	*/

void drawChart(int nsamples) {
	int band_width = floor(_width / nsamples);
		/*
		バーの幅
		floor：()内の値以下で最大の整数を返す。例えばfloor(3.7)=3、floor(-3.7)=-4となる。
		*/
	int band_pad = band_width - 1;
		/*
		グラフを描画するときバーとバーの間に隙間を入れるためにバーの幅から-1する。
		*/

	for (int band = 0; band < nsamples; band++) {
		int hpos = band * band_width + X0;
			/*
			グラフの各バーの左端のx座標。
			*/
		float d = vReal[band];
		if (d > dmax) d = dmax;
    int h = (int)((d / dmax) * (_height));
		M5.Lcd.fillRect(hpos, _height - h, band_pad, h, WHITE);
			/*
			M5StackのLCD画面に矩形を描画するための関数呼び出し。引数は(グラフの左上端のx座標, グラフの左上端のy座標, バーの横幅, バーの縦幅, バーの色)
			Question2(2)._heightが謎なので_height-hが何を表してるかいまいちよくわからない。
			*/
    if ((band % (nsamples / 4)) == 0) {
			M5.Lcd.setCursor(hpos, _height + Y0 - 10);
			M5.Lcd.printf("%.1fkHz", ((band * 1.0 * SAMPLING_FREQUENCY) / FFTsamples / 1000));
				/*
				グラフの横軸に周波数を記載する。
				SAMPLING_FREQUENCYをfsとすると、この場合、0Hz,(1/8)fsHz,(1/4)fsHz,(3/8)fsHzがそれぞれの対応する位置に記載される。
				*/
		}
	}
}

void setup() {
	/*
	setup：起動した最初に一度だけ実行される。
	*/
	M5.begin();
		/*
		M5Stackの初期化。
		*/
//  M5.Speaker.write(0); // スピーカーをオフする
		/*
		スピーカーの出力を0にすることで音声出力を無効化している。
		*/
//	M5.Lcd.setBrightness(20);
		/*
		ディスプレイの明るさを設定する。
		*/
	pinMode(MIC, INPUT);
		/*
		特定のピン(この場合MIC)の入出力を設定する。
		*/

	sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
		/*
		サンプリング周波数を用いて周期を計算している。単位をマイクロ秒にするために1000000かける。
		round：小数点以下で四捨五入する。
		*/
	M5.Lcd.printf("%d", SAMPLING_FREQUENCY);
		/*
		M5StackのLCDディスプレイにサンプリング周波数を表示する。
		printf：フォーマットされた文字列を出力するために使われる関数。
		"%d"：引数として渡された整数を文字列に変換して表示するフォーマット指定子。
		Question3.どこにも表示されていないのはなぜ？
		A.loop関数で毎回画面背景を一面黒くしてるからそこで消える。
		Q.どこに入れれば表示される？
		A.drawChart。けど毎回表示すると時間かかるしそこまで重要な情報でもないから確認のためにTERMINALに表示させるだけでいいや～。printfだけで書くと可能(以下の文)。
		*/
	printf("%d\n", SAMPLING_FREQUENCY);
}

void DCRemoval(double *vData, uint16_t samples) {
	/*
	直流の影響をなくす。
	*/
	double mean = 0;
	for (uint16_t i = 1; i < samples; i++) {
		mean += vData[i];
	}
	mean /= samples;
	for (uint16_t i = 1; i < samples; i++) {
  	vData[i] -= mean;
	}
}

//uint8_t t = 0;
	/*
	Question4.これは何？tどこ？
	A.いらない。
	*/

void loop() {
  sample(FFTsamples);
  DCRemoval(vReal, FFTsamples);
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // 窓関数
	/*
	端を滑らかにすることで影響を小さくする。
	*/
  FFT.compute(FFT_FORWARD); // FFT処理(複素数で計算)
  FFT.complexToMagnitude(); // 複素数を実数に変換
  M5.Lcd.fillScreen(BLACK);
	/*
	画面の背景を黒(BLACK)にする。
	*/
  drawChart(FFTsamples / 2);
	/*
	サンプリング定理二よりFFTsamples / 2以上の周波数の情報はとれないからFFTsamples / 2までのグラフを描画する。
	*/
}