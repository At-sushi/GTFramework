# Test

単体テスト用のdirectoryです。

単体テストフレームワークには、[iutest](https://github.com/srz-zumix/iutest)を利用しています。

## ビルド方法

### 通常のビルド

CMakeが必要です。

```sh
$ mkdir -p build
$ cd $_
$ cmake -DCMAKE_BUILD_TYPE=Debug ..
$ make
```

Debugビルドではないときは`-DCMAKE_BUILD_TYPE=Debug`を取り除いてください

### coverageを有効にする

- gcc
- gcov
- lcov
- perl

が必要です。

Windowsではmsys2のMINGW64 shellで

```sh
$ pacman -S base_devel mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake git
$ cd ~
$ git clone https://github.com/Alexpux/MINGW-packages.git --depth=1
$ cd MINGW-packages/mingw-w64-lcov
$ makepkg -cs
$ pacamn -U mingw-w64-x86_64-lcov-1.13-1-any.pkg.tar.xz
```

としてください。`mingw-w64-x86_64-lcov`のpacmanから落ちてくる`1.12-1`は壊れています(ref https://github.com/Alexpux/MINGW-packages/issues/3156)。

```sh
$ mkdir -p build
$ cd $_
$ cmake -DCMAKE_BUILD_TYPE=Debug -DGTF_Test_ENABLE_COVERAGE=1 ..
$ make
$ make GTF_Test_coverage
```

msys2では`make`の代わりに`ninja`を利用しないでください。`pacamn -U mingw-w64-x86_64-lcov-1.13-1-any.pkg.tar.xz`で`/mingw64/bin/lcov.exe`ではないく`/mingw64/bin/lcov`がインストールされるため、Windowsがこれを実行ファイルと認識できないためです。

gcov, lcovが見つからないというエラーが出る場合は、`-DGCOV_PATH=<path/to/gcov>`/`-DLCOV_PATH=<path/to/lcov>`を`cmake`に指定してください
