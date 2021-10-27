# ACM-WaveConverter
[ACM(Audio Compression Manager)](https://en.wikipedia.org/wiki/Windows_legacy_audio_components#Audio_Compression_Manager)を使用し、もともとのWaveファイルを、44100Hz/16bitのPCMのWaveファイルに変換するプログラムです。

もともと、ACMでのリサンプリングを試したくて作成したものですので、実用性は求めていません。  
(AviUtlのリサンプリングがおかしい問題の検証用です)

## ビルド方法
```
gcc -Wall -o converter.exe converter.c -lkernel32 -luser32 -lmsacm32 -lwinmm
```
などを用いてください。

## 使用方法
```
converter.exe <FilePath>
```
です。

変換前のファイルがhoge.wavである場合、元のファイルが存在するフォルダにhoge_2.wavとして生成されます。

## 追記
これの作成後に[こんな記事](https://ftvoid.com/blog/post/530)を見つけました。
