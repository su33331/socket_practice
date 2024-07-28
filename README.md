24.07.28
OS / Network / Threading を使うプログラミングをしてみましょう。
↓やること

クライアントとサーバーの C 言語のプログラムを書く。
socket API を用いる。エラー対応を必ず行う。
サーバーは、初めの(TCP)接続には1, 次の接続には2, .... と重複せずに数字を返す。
サーバー・クライアントは共に1 process で、複数スレッドを用いて、できる限り多くのリクエストを処理できる形で実装する。

使うシステムコール
socket, bind, listen, accept, write, read, close
pthread_create, etc..
