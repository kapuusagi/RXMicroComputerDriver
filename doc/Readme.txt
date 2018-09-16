目的
　　ちょっとしたRXマイコンのソフトウェアを作る時用のコードです。
　　PDG2からソースを持ってくるのがめんどいとか、
　　スマートコンフィグレータがサポートしてないとか、
　　スゴイOSだと何やっていいのかよくわからん時用。
　　Runesasさんのマイコンと開発環境はうまくできていて、
　　マイコンが変わってもある程度は流用できるような書き方ができる。

使った環境
  e2studio + rxcc

使ったハードウェア
  Runesas RPBRX65N
  https://www.renesas.com/jp/ja/products/software-tools/boards-and-kits/renesas-promotion-boards/rx65n-envision-kit.html#productInfo

どうやって使うの？
　　drv以下のソースとrx_utils以下のソースをコピーして修正して使います。

その他メモ
　普通に楽に製品を作りたかったら、素直にRunesasさんが提供しているツールを使いましょう。
　ちょっと厄介だと思ったのは、製品によって最適そうな環境が異なること。

　RX63Nなど、スマートコンフィギュレータが使えない場合
　　Pheripheral Driver Generator ＋ HEWが楽。
　　Pheripheral Driver Generator ＋ e2studio＋rxccでもいいが、
　　手順が面倒。

　RX65Nなど、スマートコンフィギュレータが使える場合
　　スマートコンフィギュレータ＋CS+
　　もしくは
　　スマートコンフィギュレータ＋e2studio ＋ rxcc

e2studio+gccの組み合わせはとっても素敵だが、
受託開発（ソースをお客さまに収める場合）などでは使いにくい。
（えっGCC？とか言われる）

