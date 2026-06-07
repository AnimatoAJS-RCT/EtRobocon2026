要素技術とモデルを開発に使おう
付録
ETロボコン技術教育資料

ETロボコン実行委員会

Rev. beb26.5.0, 2026-05-08 18:22:56 作成

目次

はじめに . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .  1
この教材の目的 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .  1

要素技術とモデルを開発に使おう 付録 の役割 . . . . . . . . . . . . . . . . . . . . . .  1

アイコンの説明（admonition） . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .  1

権利と商標 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .  2

付録A クラス図とC++ プログラミング

この教材が想定していることがら . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .  3
. . . . . . . . . . . . . . . . . . . .  5
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .  5

A.1 クラス図とコードの対応

A.2 クラスの関係に対応するコードの例を示す . . . . . . . . . . . . . . . . . .  8

A.3 依存関係（Dependecy）

. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .  8

A.4 関連関係（Association） . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .  10
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .  12
A.5 集約（Aggregation）

A.6 コンポジション関係（Composition）

. . . . . . . . . . . . . . . . . . . .  14

A.7 汎化関係（Generalization）

もしくは継承関係（Inheritance） . . . . . . . . . . . . . . . . . . . . . . . . . . .  16
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .  18

A.8 インターフェイス関係

A.9 操作区画内と属性区画の記法

付録B 状態マシン図とC++ プログラミング

. . . . . . . . . . . . . . . . . . . . . . . . . . . . .  21
. . . . . . . . . . . . .  25
B.1 状態マシン図 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .  25

B.2 Stateパターン

. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .  30

B.3 StateパターンのStateMachine状態遷移図 . . . . . . . . . . . . . . . . .  31

B.4 StateMachineクラス図をステートパターンで実装

. . . . . . . . .  37

この教材の目的

はじめに

この教材の目的

1. この教材は、 ETロボコン に参加されるみなさんに、モデル作成に必要となる知識やスキル取得の機

会を提供することを目的に作成しています。

2. この教材は、「 権利・諸注意 」 記載の事項を理解、遵守の上、活用してください。

要素技術とモデルを開発に使おう 付録 の役割

3. この教材は、ETロボコンに参加されるみなさんに、モデル作成に必要となる知識やスキル取得の機会

を提供することを目的に作成しています。

アイコンの説明（admonition）

4. 読者に情報や注意を促したいことがあって、それを目立たせたいときは、次のよう

な「Admonitions」アイコンを用います。

告知（NOTE）。みなさんが覚えておくとよい追加情報など。

ティップ（TIP）。覚えておくと便利なちょっとしたヒントやコツ。

重要（IMPORTANT）。見落とすと期待通りの結果が得られないことがら。

注意（CAUTION）。慎重に行動する（注意を払う）ことを勧めることがら。

警告（WARNING）。守らないと破損や怪我などにつながることがら。

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 1

はじめに

権利と商標

権利・諸注意

• この教材は ETロボコン実行委員会 が制作したもので、この教材に関する権利、責任は ETロボコ

ン実行委員会 が保有します。

• この教材に記載されている情報は、 2026年05月 現在のものであり、URLなどの各種の情報や内

容は、ご利用時には変更されている可能性があります。

• この教材の内容は参照用としてのみ使用されるべきものであり、予告なしに変更されることがあ

ります。

• ETロボコン実行委員会 はこの教材の内容を保証するものではありません。この教材の内容に誤り
や不正確な記述がある場合やこの教材に記載されている内容の運用によっていかなる損害が生じ

た場合も、 ETロボコン実行委員会 は一切の責任を負いかねます。あらかじめご了承ください。
• この教材は、 ETロボコン の参加資格（企業・大学・個人）の範囲内に限り、ご自由に活用してい

ただいてかまいません。

• ETロボコン の参加資格を保有しない場合、本教材のいかなる部分についても、 ETロボコン実行
委員会 との書面等による事前の同意なしに、電気、機械、複写、録音その他のいかなる形式や手

段によっても、複製および検索システムへの保存や転送は禁止します。

商標等について

• LEGO、LEGOロゴ、MINDSTORMS、MINDSTORMSロゴは、LEGOグループの商標または著作

権です。

• Microsoft、Windows及びWindowsロゴは、マイクロソフト企業グループの商標です。
• astah、 Astah* は、株式会社チェンジビジョンの、日本、米国、欧州における登録商標です。
• Apple、iCloud、iPad、iPhone、Mac、Macintosh、macOSは、米国およびその他の国々で登

録されたApple Inc.の商標です。

• AMAZON、Amazonは、Amazon Services LLCおよびその関連会社の商標です。
• Excel、PowerPoint、Wordは、米国Microsoft Corporationおよび／またはその関連会社の商

標です。

• Linuxは、Linus Torvalds氏の日本およびその他の国における登録商標または商標です。
• その他、この教材に記載されている社名、製品名、ブランド名、システム名などは、一般に商標

または登録商標でそれぞれの帰属者の所有物です。
◦ 本文中では © 、® 、 ™ 、は表示していません。

2 | ETロボコン技術教育資料

Rev. beb26.5.0

この教材が想定していることがら

この教材が想定していることがら

想定する受講者

5. C言語/C++言語あるいはこれらに類似の言語のプログラミング経験はあっても、ソフトウェアの分析

・設計にモデルを使った開発経験のない開発者のみなさん。

トレーニングのゴール

1. 環境構築からソースコードのコンパイル、実機での実行方法までの一連の作業の流れを知ってい

る。

2. モデルとコードのつながりを知っている。
3. 要素技術をモデルに組込んで使える。

6. 開発プロセスに沿って、課題を要求として整理し、そこから設計、実装へと開発を進める方法につい

ては、後続の技術教育「開発プロセスに沿って開発する」で演習します。

トレーニングの進め方

• この教材ではRasPike-ART環境の実機を使って説明・演習します。
◦ シミュレーターやSPIKE-RTモードを使う場合、演習環境や実行方法は異なりますが、作成す

るコードは同じものが使えます。

• コードの状況をモデルで表すことで、モデルは何を表せばよいのか、何を表せるのかを実感しま

す。

• モデルとコードを並行して編集し、モデルとコードの対応づけを学びます。
• 要素技術については、実験してからモデルに組込み、そのモデルからコードを作成します。

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 3

はじめに

4 | ETロボコン技術教育資料

Rev. beb26.5.0

A.1 クラス図とコードの対応

クラス図とC++ プログラミング

付録A

A.1 クラス図とコードの対応

7. クラス図は、システム内の各クラスについて、その役割、責務、インターフェイスに注目し、クラス

内部の実装詳細を隠蔽した「ブラックボックス」として捉えることで、システムを構造に関して抽象

化します。 この抽象レベルでの検討は、クラス間の連携、責務分担、依存関係などの構造を図式化、

視覚化することで、実際にコードを書く前にアーキテクチャの健全性を確認するうえで非常に重要で

有効な検討になります。 チーム内で設計思想（設計方針や仕様）を共有し、具体的な実装コードに先

立ち、各自の理解を統一するための「共通言語」となり、実装前の合意形成ができます。 このクラス

図とオブジェクト指向プログラミング（OOP）との対応づけには、以下の点で大きな意義と必要性が

あります。

• コードとの対応づけは、抽象設計と具体実装の橋渡しになります。このとき、設計した情報が実

装において失われないようにすることが必要です。

• 設計とコードが対応していれば、レビュー時に設計意図を確認しながらコードを修正でき、誤解
や重複作業が減ります。逆を言えば、実装の構造や振舞いが、設計の構造や振舞いに立ち戻るこ

とができることになります。

• コードとの対応づけにより、オブジェクト指向設計の概念と実際のプログラムコードの関係を直

感的に理解しやすくなります。

8. 設計とコードの対応づけができれば、つぎのような効用があります。

9. 実行時動作のシミュレーションによる検証

クラス図を基にOOPの実装を進めることで、各クラスのインターフェースや連携パターンが、実際の

プログラミング環境でどのように振る舞うかを、プロトタイプや疑似コードを通して先行検証できま

す。 つまり設計段階の仮説を実際にコードを実行した際の動作検証が容易になります。

10. 設計意図の実装反映とプロセスの最適化

クラス図を基にOOPの実装を進めることで、設計の意図を保持したままコード実装へ移行できます。

実装すべきフレームワークやシステムの大枠がプロトタイプや疑似コードを通して事前に設計意図の

検証が可能になります。 つまり設計から実装へのシームレスな移行が容易になります。

11. OOPの実践的な適用と検証

クラス図とOOPの対応づけを行うことで、抽象化された設計が具体的なコード（OOP）に落とし込ま

れる際の実現可能性が判断できます。 オブジェクト指向の「カプセル化」、「抽象化」、「継

承」、「ポリモーフィズム」が正しく機能するかを検証することができます。 また、オブジェクト間

の「メッセージパッシング」や動的な振舞い、インスタンス生成のタイミングなど、実行時の動作に

ついても、プロトタイピングによるシミュレーションを通して評価することができます。

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 5

付録A クラス図とC++ プログラミング

12. 前述する通り、設計のクラス図とコードの対応づけの効用があります。しかし、クラス図はあくまで

静的な観点からシステムを捉えるため、動的な動作や実行時の振舞いについては十分に表現できない

という限界もあります。これらの面を補うために、アクティビティ図、シーケンス図、状態マシン図

などの動的なモデル図を併用します。なお、状態マシン図とコードとの対応づけを 付録B に示しま

す。

A.1.1 クラスの操作と C++ のメンバ関数の対応づけ

13. UMLクラス図のクラスの操作と、 C++ のクラスのメンバ関数を対応づけます。 操作の引数が、C++

のメンバ関数の仮引数に対応します。 操作の可視性が、 C++ のアクセス指定子に対応します。

表 A.1 アクセス指定子のUML記述とC++記述と意味

UML

+

-

#

C++

public

意味

すべてのクラスからアクセス可能

private

そのクラス自身からのみアクセス可能

protected

サブクラスまたはそのクラス自身からのアクセス可能

package（UML記述：~） の意味は、「同一パッケージのクラスまたはそのクラス自身からのア

クセス可能」です。C++では、この定義はありません。

表 A.2 アクセス指定子とアクセス制御範囲

アクセス指定子

クラス自身

サブクラス

クラス外

public

private

protected

○

○

○

UMLのクラスの操作

○

×

○

○

×

×

可視性 操作名 (引数 : 引数の型) : 戻り値の型

例 A.1 UMLのクラスの操作の例

+ operation( param : int ) : int ①

UML

① 操作名が operation 、引数名が param 、引数の型が int 、戻り値の型が int

C++ のメンバ関数（クラス定義中で関数を実装する場合）

アクセス指定子: 戻り値の型 メンバ関数名(仮引数の型 : 仮引数名） { 処理 }

6 | ETロボコン技術教育資料

Rev. beb26.5.0

A.1 クラス図とコードの対応

例 A.2 C++ のメンバ関数の例（クラス定義の中で関数を実装する場合）

class クラス名{
  public: ①
  int operation( int param ){ ②
    処理;
  }
};

① アクセス指定子は public

② 戻り値の型は int 、メンバ関数名は operation 、仮引数の型は int 、 仮引数名は param

C++ のメンバ関数（クラス定義と関数の実装を分ける場合）

戻り値の型 クラス名::名前(仮引数の型 仮引数名) { 処理 }

例 A.3 C++ のメンバ関数の例（クラス定義と関数の実装を同じファイル内で分ける場合）

class クラス名{
  public:
  int operation( int param );
};

int クラス名::operation( int param ) { ①
  処理;
}

① メンバ関数名が operation 、戻り値の型が int 、仮引数名が param

CPP

CPP

メンバ関数の実装をクラス定義と分ける方法を使うと、メンバ関数の処理が長い場合にクラス

定義の見通しが悪くなるのを避けられます。

A.1.2 クラスの属性と C++ のメンバ変数の対応づけ

14. UMLクラス図のクラスの属性と、 C++ のクラスのメンバ変数を対応づけます。 クラスの属性の型

が、C++ のメンバ関数の型に対応します。 属性の可視性が、 C++ のアクセス指定子に対応します。

クラスの属性に多重度が定義されている場合は、メンバ変数を配列や、new式によるヒープの確保

や、vector` 等のシーケンスコンテナを使って対応づけます。

クラスの属性

可視性 属性名: 属性の型 多重度

例 A.4 クラスの属性の例

- attr : int [10] ①

UML

① 可視性が - （private）、属性名が attr 、属性の型が int 、多重度が指定されていて、厳密

に10（もし1以上10以下なら [1..10]）

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 7

C++ のメンバ変数（配列に対応づける場合）

可視性: メンバ変数の型 メンバ変数名[要素数];

例 A.5 C++ のメンバ変数の例（配列に対応づける場合）

class クラス名{
  private: ①
  int param[10]; ②
};

① アクセス指定子は public

付録A クラス図とC++ プログラミング

CPP

② メンバ変数の型は int 、メンバ変数名が para 、この変数は配列になっていて要素数は 10

操作区画内の記法、属性区画内の記法の詳細は、A.9 に示します。

A.2 クラスの関係に対応するコードの例を示す

15. UMLにはクラスの関係を表現する方法が5種類あります。この5種類はクラスの「依存による関係」

の強さで説明することができます。 ２つのクラスの依存による関係の強さとはクラス同士が互いにど

のように依存しているかによって決まります。 依存の弱いクラスの関係から依存の強いクラスの関係

を並べると「依存」「関連」「集約」「コンポジション」「継承」になります。 なお「継承」に

は、「抽象」や「インターフェース」が含まれます。

16. 分析クラス図から設計クラス図が導出され、最終的には実装クラス図が導出されます。 実装クラス図

に現れたクラスを最終的にはプログラミングします。 このプログラミングのポイントは、クラスの関

係を仕組みとして作り込むということです。 ここでは、前述しているとおり、C++プログラミング言

語でプログラミングします。 他のオブジェクト指向プログラミング言語については、ご自身で真似て

置き換えてみてください。

A.3 依存関係（Dependecy）

17. 依存関係は、あるモデル要素（クライアント）が別のモデル要素（サプライヤ）の存在または特性を

使用あるいは依存していることを示す関係です。 依存関係は、クライアントに対するサプライヤの変

更がクライアントに影響を与える可能性があることを意味します。

18. 依存関係の概要は以下の通りです。

• 依存関係は破線の矢印で表現され、クライアントからサプライヤの方向を指します。
• 依存関係には、その性質をより具体的に示すためのステレオタイプ（キーワード）が付与される

ことがあります。

8 | ETロボコン技術教育資料

Rev. beb26.5.0

A.3 依存関係（Dependecy）

A.3.1 引数によるインスタンスの受け渡し

19. プログラミングにおいて、ある操作を呼び出す際にその操作が処理するために必要な情報を引数とし

て渡せます。 この引数として、あるクラスのインスタンス（オブジェクトとも呼びます）を渡すこと

もできます。

20. このつながりは、クラス図において明示的に関連として現れません。 しかし、操作の引数としてイン

スタンスを渡すことは、プログラミングの重要な方法でもあります。

21. あるクラスの操作が別のクラスのインスタンスを引数として受け取る場合、受け取る側をクライアン

ト、引き渡すインスタンスの元になるクラスをサプライヤーと呼ぶとすると、これらの関係は 図 A.1

のような依存関係で表すことができます。

Client

Supplier

図 A.1 クラス間の依存関係の例

22. 以下に、C++ 言語で「引数としての受け渡し」による依存関係の具体的なコード例を示します。

リスト A.1 Dependecy_01.hpp

CPP

 1 #include <iostream>
 2
 3 class Supplier { ①
 4 public:
 5     void showMessage() {
 6         std::cout << "Class Supplier: Message from Supplier." << std::endl;
 7     }
 8 };
 9
10 class Client { ②
11 public:
12     void useSupplier(Supplier& b) { ③
13         std::cout << "Class Client: Calling Supplier's method." << std::endl;
14         b.showMessage();
15     }
16 };
17
18 int main() { ④
19     Supplier b;
20     Client a;
21     a.useSupplier(b); // クライアントがサプライヤに依存している
22     return 0;
23 }

① Supplier クラスは、 showMessage 操作を持つ単純なクラスです。

② Client クラスは、 useSupplier 操作を持つクラスです。

③ useSupplier 操作は、 Supplier クラスのインスタンスを引数として受け取ります。この引数によっ

て、2つのクラスの間に依存関係が生じています。

④ main関数は、 Client クラスと Supplier クラスのインスタンスを作成し、 Client クラスの

useSupplier 操作を通じて、 Supplier クラスの showMessage 操作を呼び出しています。

23. リスト A.1 では、 Client クラスは、 Supplier クラスの存在なしに動作できません。 つまり、 Client

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 9

付録A クラス図とC++ プログラミング

クラスは Supplier クラスに依存しています。

A.4 関連関係（Association）

24. 関連関係は、異なるクラスのインスタンス間における構造的なつながりを表します。 これは、一方の

クラスのオブジェクトが他方のクラスのオブジェクトと何らかの形で関係を持つことを意味します。

関連は、オブジェクトが互いにメッセージを送信したり、操作を呼び出したり、互いに認識し合った

りするなど、様々な形で現れます。

25. 関連関係は、以下のような特性を持ちます。

多重度（Multiplicity）

関連するインスタンスの数を指定します（例：1対1、1対多、多対多）。

誘導可能性（Navigability）

一方のオブジェクトから他方のオブジェクトへの参照が可能かどうかを示します。

誘導不可能（Non-navigable）

一方のオブジェクトから他方のオブジェクトへの参照が不可能ということを示します。

役割（Role）

関連の各端におけるクラスの役割を記述します。

集約（Aggregation）とコンポジション（Composition）

より強い形式の関連であり、全体と部分の関係を表します。

関連クラス（Association Class）

関連自体が属性や操作を持つ場合に用いられます。

26. 集約（Aggregation）については A.5 で、コンポジション（Composition）については A.6 で説明し

ます。

図 A.2 クラス図の関連関係の特性

【コラム 1 】 コラム：関連クラスの設計と実装

2つの要素間の関係は、必ずしも単純な構造の関係とは限りません。 2つの要素間の関係の関連

自体がいろいろなデータや振る舞いをもつような複雑な場合もあります。 このような複雑な場

合に、関連を説明する「関連クラス」で設計します。

関連クラスには「それ自身と何かの間で定義できない」という制約があります。 この制約のた

10 | ETロボコン技術教育資料

Rev. beb26.5.0

A.4 関連関係（Association）

め、関連クラスのままプログラムに実装することを示している書籍もありますが、わたしはお

勧めしません。 「実装に使う言語には関連クラスをそのまま実装する方法がない」と思っても

らって構いません。

それは、関連クラスが関連に関係する「属性」とクラスに関係する「属性」を同時に持ち合わ

せていますが、これらが明示的にモデルに現れていません。また関連クラスの「操作」も明示

的ではありません。ですから明示されていないものをプログラムにすることは難しいことなり

ます。設計者のモデルの意図を見落とすかもしれません。 しかし、クラスになっていれば、こ

の後に解説する関連などの関係になりますので、プログラムに容易く変換できます。

そのためには、クラスとクラスを紐づける関連クラスとしての働きを持たせるようにクラス化

することで、より関連に関する「情報の体系的な管理の属性」「操作の実装」をモデル化する

ことできます。 これらが明示的になったクラスになりますのでプログラムにも展開しやすくな

ります。

なお、関連クラスのクラス化は次のようにします。 クラスAとクラスBの関連に対応する関連

クラスは、クラスAとクラスBの間に関連クラスをクラスCに変換して、クラスAとクラスC、ク

ラスCとクラスBの関連関係で組み換えます。 あとはUMLのクラスとC++のクラスの対応づけに

従って実装します。

27. C++ においては、関連関係の種類や多重度、誘導可能性に応じて、 図 A.3 に示すクラス図や、 リスト

A.2 に示すコードのように表現されます。

Client

roleB

Supplier

+ useSupplier ()

1

1

+ showMessage ()

図 A.3 関連に誘導可能性や関連端名を使ったクラス図の例

28. リスト A.2 に、C++ 言語で関連関係を示す具体的なコード例を示します。

リスト A.2 Assosciation_01.hpp

CPP

 1 #include <iostream>
 2
 3 class Supplier {
 4 public:
 5     void showMessage() {
 6         std::cout << "Class Supplier: Message from Supplier." << std::endl;
 7     }
 8 };
 9
10 class Client {
11 private:
12     Supplier roleB; ①
13 public:
14     void useSupplier() { ②
15         std::cout << "Class Client: Calling Supplier's method." << std::endl;
16         roleB.showMessage();
17     }
18 };

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 11

付録A クラス図とC++ プログラミング

19
20 int main() {
21     Client a;  ③
22     a.useSupplier(); ④
23     return 0;
24 }

① クラスClientは、クラスSupplierのインスタンス roleB をメンバ変数として保持します。

② useSupplier() 操作で b.showMessage() 操作を呼び出し、Supplierの機能を利用します。

③ main() 関数で Client のオブジェクトを作成し、useSupplier() を実行します。

④ ClientがSupplierに依存しています。

29. この方法では、クラスClientがクラスSupplierに強く結びついており、Clientのインスタンスが生成

され、Supplierも自動的に生成されます。

ここでは関連関係を単純な「実体を持つケース」で実装しています。「実体を持たないケース

（ポインタまたはリファレンス）」で実装しても良いです。

A.5 集約（Aggregation）

30. 集約は、クラス図における関連関係の一種であり、「全体と部分」の関係を表します。

31. これは、一方のクラス（全体）のインスタンスが他方のクラス（部分）のインスタンスを含むか、ま

たは関連を持つことを意味しますが、部分のインスタンスは全体のインスタンスとは独立して存在で

きる点が特徴です。

32. これは集約の部分が複数の全体で共有される可能性がある関係を示します。 UMLの表記では、部分

にあたるクラス側の関連端に白抜きの菱形を描きます。

Team

- players

Player

- name: string

+ showPlayers ()

1

0..10

+ getName () : string

図 A.4 集約を使ったクラス図の例

33. リスト A.3 に、C++ 言語で集約を示す具体的なコード例を示します。

リスト A.3 Aggregation_01.hpp

CPP

 1 #include <iostream>
 2 #include <string>
 3
 4 class Player { ①
 5 public:
 6     std::string name;
 7
 8     Player(std::string n) : name(n) {}
 9
10     std::string getName() const {
11         return name;
12     }
13 };
14

12 | ETロボコン技術教育資料

Rev. beb26.5.0

A.5 集約（Aggregation）

15 class Team {
16 public:
17     Player* players[10]; ②
18     int playerCount = 0; // 現在のプレイヤー数
19
20     Team() {
21         for (int i = 0; i < 10; ++i) {
22             players[i] = nullptr; // 初期状態ではすべてnullptr
23         }
24     }
25
26     void addPlayer(Player* p) { ③
27         if (playerCount < 10) {
28             players[playerCount++] = p;
29         } else {
30             std::cout << "チームにはこれ以上追加できません！\n";
31         }
32     }
33
34     void removePlayer(Player* p) { ④
35         for (int i = 0; i < playerCount; ++i) {
36             if (players[i] == p) {
37                 for (int j = i; j < playerCount - 1; ++j) {
38                     players[j] = players[j + 1]; // 配列を詰める
39                 }
40                 players[playerCount - 1] = nullptr;
41                 playerCount--;
42                 return;
43             }
44         }
45     }
46
47     void showPlayers() const {
48         for (int i = 0; i < playerCount; ++i) {
49             if (players[i] != nullptr) {
50                 std::cout << "プレイヤー: " << players[i]->getName() << std::
   endl;
51             }
52         }
53     }
54 };
55
56 int main() {
57     Player p1("Player1"), p2("Player2"), p3("Player3"); ⑤
58
59     Team team; ⑤
60     team.addPlayer(&p1); ⑥
61     team.addPlayer(&p2);
62     team.addPlayer(&p3);
63
64     std::cout << "チームのメンバー:\n";
65     team.showPlayers();
66
67     std::cout << "\nPlayer2 を削除\n";
68     team.removePlayer(&p2);  ⑦
69
70     std::cout << "チームのメンバー:\n";
71     team.showPlayers();
72
73     // Player2を確認する
74     std::cout << "プレイヤー２: " << p2.getName() << std::endl;
75
76     return 0;
77 }

① Player インスタンスは独立して作成され、Team とは別に存在可能です。

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 13

付録A クラス図とC++ プログラミング

② Team は Player* の配列を持ち、プレイヤーのアドレスを保持（ポインタ配列）します。

③ addPlayer 操作で Player をチームに追加します。

④ removePlayer 操作で Player をチームから削除します。しかしプレイヤー自体は削除されません。

⑤ main 関数では、Player と Team のインスタンスを作成します。

⑥ Team のインスタンス team に、Player のインスタンスを追加します。

⑦ Team のインスタンス team から、Player のインスタンスを削除します。

34. このプログラムでは、Player インスタンスは Team から削除されても、他の Team に追加されたり、単

独で存在することができます。

A.6 コンポジション関係（Composition）

35. コンポジション関係は、クラス図における集約の一種であり、より強い形式の「全体と部分」の関係

を表します。

36. コンポジションは、一方のクラス（全体）のインスタンスが他方のクラス（部分）のインスタンスを

排他的に所有する強い関連です。 「部分」のインスタンスのライフサイクルは「全体」のインスタン

スに依存します。 つまり、「全体」のインスタンスが破棄されると、それに含まれる「部分」のイン

スタンスも同時に破棄されます。

37. 「全体」クラスは「部分」クラスのインスタンスを物理的に包含することが一般的です。

38. コンポジションの関係では、「全体」クラス側の多重度が 1 または 0..1 であることが多いです。 こ

れは、「部分」が特定の「全体」に属することを強調するためです。 「部分」クラス側の多重度は通

常 1 以上になります。

39. UMLのクラス図では、「全体」のクラス側の関連端に黒塗りの菱形を描いて、コンポジション関係

を示します。

40. 図 A.5 に、コンポジション関係のクラス図を示します。

Team

- players

Player

- name: string

+ showPlayers ()

1

10

+ getName () : string

図 A.5 コンポジションを使ったクラス図の例

41. リスト A.4 に、C++ 言語でコンポジション関係を示す具体的なコード例を示します。

リスト A.4 Composition_01.hpp

CPP

 1 #include <iostream>
 2 #include <string>
 3
 4 class Player {
 5 public:
 6     std::string name;
 7
 8     Player(std::string n) : name(n) {}
 9

14 | ETロボコン技術教育資料

Rev. beb26.5.0

A.6 コンポジション関係（Composition）

10     std::string getName() const {
11         return name;
12     }
13 };
14
15 class Team {
16 public:
17     Player players[10] = { ①
18         Player("Player1"), Player("Player2"), Player("Player3"), Player(
   "Player4"),
19         Player("Player5"), Player("Player6"), Player("Player7"), Player(
   "Player8"),
20         Player("Player9"), Player("Player10")
21     };
22
23     void showPlayers() const {
24         for (const Player& p : players) {
25             std::cout << "プレイヤー: " << p.getName() << std::endl;
26         }
27     }
28 };
29
30 int main() {
31     Team team; ②
32     team.showPlayers();
33     return 0;
34 }

① Team の players 配列に Player のインスタンスを直接持たせます。

Player のインスタンスは Team の内部に組み込まれ、外部から独立して生存できません。

② Team が破棄されると、自動的に Player も破棄されます（生存期間が一致）。

コンポジションの実装と振舞い

1. 全体（ Team ）が消滅すれば、部分（ Player ）も消滅します。
◦ Player の配列は Team のメンバ変数として直接保持されますので、Team が破棄されますと

Player も自動的に破棄されます。

◦ Player は new で動的に確保されていませんので、delete の管理も不要です。
2. 部分（ Player ） は独立して存在できません。
◦ Player のインスタンスは Team の配列に直接含まれますので、Player の寿命は Team の寿命と一

致します。

◦ Team が破棄されますと、Player も一緒に破棄されます。

42. このように、C++ ではオブジェクトを直接メンバとして持つことで、クラス図におけるコンポジショ

ンの概念をコードとして具体的に実現できます。 重要なのは、「全体」クラスが「部分」クラスのオ

ブジェクトをポインタや参照ではなく直接所有し、そのライフサイクルを管理することです。 これに

より、「全体」と「部分」が不可分な関係となり、『「全体」がなくなると「部分」も必然的に消滅

する』というコンポジションの特徴を表現できます。

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 15

付録A クラス図とC++ プログラミング

A.7 汎化関係（Generalization） もしくは継承関係（Inheritance）

43. クラス図における継承元の、より一般化されたクラス（矢印側のクラス）を「親クラス」「基底クラ

ス」「スーパークラス」といいます。 もう一方のクラスを「子クラス」「派生クラス」「サブクラ

ス」といいます。 汎化（もしくは継承関係）はスーパークラスの特性（属性や操作など）をサブクラ

スが引き継ぐ関係を表します。 サブクラスはスーパークラスを特化した概念を表し、独自の特性を持

つことができます。 サブクラスがスーパークラスの一種であるという関係性を意味します。

44. 汎化（もしくは継承関係）を利用することで、 スーパークラスで定義された属性や操作をサブクラス

で再定義（オーバーライド）したり、追加したりすることができます。 これにより、コードの再利用

性が高まります。

45. スーパークラスが抽象クラスである場合、そのクラスのインスタンスを直接作成することはできませ

ん。 抽象クラスは、サブクラスに共通のインターフェイスを定義するために使用されます。 抽象ク

ラスは通常、一つ以上の抽象操作（実装を持たない操作）を持ちます。

46. UMLのクラス図では、サブクラスからスーパークラスへ向けて、白抜きの三角の矢印で汎化（もし

くは継承）関係を示します。

47. 図 A.6 に、 Player をスーパークラス とし、 Goalkeeper （ゴールキーパー）と Striker（ストライカー

）をサブクラスとする汎化（もしくは継承）の例を示します。

図 A.6 継承関係のクラス図の例

48. 以下に、C++ 言語で継承関係を示す具体的なコード例を示します。

リスト A.5 Inheritance_01.hpp

 1 #include <iostream>
 2 #include <string>
 3
 4 // 基底クラス（親クラス）
 5 class Player { ①
 6 protected:
 7     std::string name; ②

CPP

16 | ETロボコン技術教育資料

Rev. beb26.5.0

A.7 汎化関係（Generalization） もしくは継承関係（Inheritance）

 8
 9 public:
10     Player(std::string n) : name(n) {}
11
12     std::string getName() const { ②
13         return name;
14     }
15
16     virtual std::string getPosition() const { ③
17         return "Unknown";
18     }
19 };
20
21 // 派生クラス（子クラス）: ゴールキーパー
22 class Goalkeeper : public Player { ④
23 public:
24     Goalkeeper(std::string n) : Player(n) {}
25
26     std::string getPosition() const override { ⑤
27         return "Goalkeeper";
28     }
29 };
30
31 // 派生クラス（子クラス）: ストライカー
32 class Striker : public Player { ⑥
33 public:
34     Striker(std::string n) : Player(n) {}
35
36     std::string getPosition() const override { ⑦
37         return "Striker";
38     }
39 };
40
41 int main() {
42     Goalkeeper gk("Player1");
43     Striker st("Player2");
44
45     std::cout << gk.getName() << " のポジション: " << gk.getPosition() << std
   ::endl;
46     std::cout << st.getName() << " のポジション: " << st.getPosition() << std
   ::endl;
47
48     return 0;
49 }

① Player クラスがスーパークラスです。

② name をメンバ変数として持ち、getName() で取得できます。

③ getPosition() は仮想関数（virtual）にし、サブクラスでオーバーライド可能にします。

virtual を使うことでポリモーフィズムを実現しています。

④ Goalkeeper は Player を継承します。

⑤ Goalkeeper は getPosition() をオーバーライドして "Goalkeeper" を返します。

⑥ Striker は Player を継承します。

⑦ Striker は getPosition() をオーバーライドして "Striker" を返します。

継承を使うメリット
• 共通の機能（getName など）を Player で定義し、コードの重複を減らせます。
• ポリモーフィズム（仮想関数）により、異なる Player の種類を統一的に扱えます。

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 17

付録A クラス図とC++ プログラミング

A.8 インターフェイス関係

A.8.1 インターフェイス関係の概要

49. UMLのクラス図におけるインターフェイスは、クラスやコンポーネントが提供するサービスの仕様

を定義するものです。 インターフェイス自身は振る舞いの実装を持たないで、操作（メソッド）のシ

グネチャ（名前、引数、戻り値の型）を定義します。 クラスは、一つまたは複数のインターフェイス

を実現（実装）することで、それらのインターフェイスが規定する振る舞いを保証します。 インター

フェイス関係は、システムの疎結合性を高め、柔軟で保守性の高い設計を可能にする重要な概念で

す。

A.8.2 インターフェイス関係の主要な側面

インターフェイス（Interface）

• Classifier の一種であり、振る舞いの仕様を定義します。
• 属性（静的属性の定数）を持つこともありますが、通常は操作（operation）の定義が主体です。
• キーワード «interface» を付与したステレオタイプで表現します。
• あるいは提供インターフェイス（ロリポップ記号）で表現されます。

インターフェイス実現（Interface Realization）

• クラスが特定のインターフェイスを実装することを表す関係です。
• 実現するクラスから実現されるインターフェイスへ向けて、破線に白抜きの三角を付けた線で表

現します。

• あるいは提供インターフェイス（ロリポップ記号）を使用する場合は、実現するクラスに実線で

接続します。

• Interface Realization [Class] は Abstraction の特殊化です。

使用（Usage）

• あるClassifier（通常はクラス）が、別のClassifier（通常はインターフェイス）が提供するサー

ビスを利用することを示す依存関係の一種です。

• 使用するClassifierから使用されるインターフェイスへ向けて、破線の矢印で表現します。
• 要求インターフェイス（ソケット記号）を使用する場合は、要求するクラスに実線で接続しま

す。

提供インターフェイス

• クラスやコンポーネントが外部に提供するインターフェイスは提供インターフェイスと呼ばれ、

実線と丸印のロリポップ記号で示されます。

18 | ETロボコン技術教育資料

Rev. beb26.5.0

A.8 インターフェイス関係

要求インターフェイス

• クラスやコンポーネントが動作に必要な他のサービスを表すインターフェイスは要求インターフ

ェイスと呼ばれ、ソケット記号で示されます。

汎化（Generalization）

50. インターフェイスのGeneralization（汎化）によって継承することもできます。

51. 「インターフェイス関係」という用語は、直接的に特定のコードを指すものではありません。 これは

主に、ソフトウェアの設計段階で使用されるUML（Unified Modeling Language）のクラス図にお

ける概念です。 インターフェイス関係は、クラスが提供するサービスの仕様を定義する「インターフ

ェイス」と、そのインターフェイスを実装するクラス、またはそのインターフェイスを使用するクラ

スの間の関係性を表します。

52. インターフェイスクラスを使うと、異なる種類のクラスで共通の操作を統一的に扱う ことができま

す。

53. C++ には Java のような 「interface」 キーワードはありませんが、抽象クラス を実装することで、

インターフェイスクラスを実現します。

モデル図には、意味するインターフェイスクラス、抽象クラスを使い分けた方がよいです。イ

ンターフェイスクラスだが、C++ なのでと言って抽象クラスを表明しない方がよいです。

54. 図 A.7 に、インターフェイス関係のクラス図を示します。

図 A.7 インターフェイス関係のクラス図の例

図 A.7は、図 A.6を単純にインターフェースとして実装する場合のひとつの考え方です。この考え

方は派生関係として捉えていません。player に名前とポジションがあるという仕様を表明した

ものです。

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 19

付録A クラス図とC++ プログラミング

リスト A.6 に、C++ 言語でインターフェイス関係を示す具体的なコード例を示します。

リスト A.6 Interface_01.hpp

CPP

 1 #include <iostream>
 2 #include <string>
 3
 4 // インタフェースクラス（純粋仮想関数を持つ）
 5 class IPlayer {
 6 public:
 7     virtual std::string getName() const = 0;   // 純粋仮想関数
 8     virtual std::string getPosition() const = 0; // 純粋仮想関数
 9     virtual ~IPlayer() {}  //
   仮想デストラクタ（派生クラスでのメモリ解放を確実にする）
10 };
11
12 // ゴールキーパー（インタフェースを実装）
13 class Goalkeeper : public IPlayer {
14 private:
15     std::string name;
16
17 public:
18     Goalkeeper(std::string n) : name(n) {}
19
20     std::string getName() const override {
21         return name;
22     }
23
24     std::string getPosition() const override {
25         return "Goalkeeper";
26     }
27 };
28
29 // ストライカー（インタフェースを実装）
30 class Striker : public IPlayer {
31 private:
32     std::string name;
33
34 public:
35     Striker(std::string n) : name(n) {}
36
37     std::string getName() const override {
38         return name;
39     }
40
41     std::string getPosition() const override {
42         return "Striker";
43     }
44 };
45
46 // インタフェースを利用する関数（ポリモーフィズムを活用）
47 void printPlayerInfo(const IPlayer& player) {
48     std::cout << player.getName() << " のポジション: " << player.getPosition() <<
   std::endl;
49 }
50
51 int main() {
52     Goalkeeper gk("Player1");
53     Striker st("Player2");
54
55     printPlayerInfo(gk);
56     printPlayerInfo(st);
57
58
59     return 0;

20 | ETロボコン技術教育資料

Rev. beb26.5.0

A.9 操作区画内と属性区画の記法

60 }

A.8.3 インターフェイスを実装するプログラムのポイント

• インターフェイスクラス（純粋仮想関数を持つクラス）を作ります。
• サブクラスがそのインターフェイスを実装します。

56. 例えば、 IPlayer インターフェイス を定義します。 - Goalkeeper（ゴールキーパー） - Striker（スト

ライカー） の 2 つのクラスがそれを実装するようにします。

IPlayer をインターフェイスクラス（純粋仮想関数を持つクラス）として定義
• getName() と getPosition() を純粋仮想関数（= 0）にすることで、サブクラスでのオーバーライド

を必須にします。

• ~IPlayer()（仮想デストラクタ）を定義し、サブクラスのデストラクタが正しく呼ばれるようにし

ます（重要）。

Goalkeeper と Striker が IPlayer を実装
• IPlayer を public 継承し、純粋仮想関数を override で実装します。

ポリモーフィズムを活用
• IPlayer のポインタや参照を使うことで、Goalkeeper や Striker を統一的に扱えます。
• printPlayerInfo() のような関数を作れば、どの IPlayer も一括して処理できます。

A.8.4 インターフェイスを使うメリット

• 異なる種類のクラスで共通の操作を定義できます。
• ポリモーフィズムを活用できます。
• 複数のクラスが共通のインターフェイスを持つことで、拡張しやすくなります。

A.8.5 まとめ

• C++ には Java のような interface はないが、純粋仮想関数を使えば同様のことができます。
• インターフェイスクラス（純粋仮想関数を持つクラス）を作ります。
• サブクラスがインターフェイスを実装し、ポリモーフィズムを活用します。
• 仮想デストラクタを定義して、正しくメモリ管理します。

A.9 操作区画内と属性区画の記法

A.9.1 操作区画内の記法

操作

可視性 名前（引数）：戻り値の型｛プロパティ｝

可視性

'+'  Public | '-'  Private |  '#' Protected | '~' Package

名前

操作を言い現わす名称とします

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 21

付録A クラス図とC++ プログラミング

引数

引数の構文要素は、A.9.2 に示します

戻り値の型

操作が返す情報の型を指定します。省略されている場合は不明となります。

プロパティ

操作に関する制約とプロパティを指定します。

プロパティは省略可能です。省略する場合は、中括弧（ {} ）も省略します。

A.9.2 引数

引数

方向 引数名: 型［多重度］＝デフォルト値｛プロパティ｝

方向

'in' | 'out' | 'inout' | 省略時のデフォルト値は、'in'

引数名

引数を言い現わす名称とします。

型

引数のタイプを指定する式です。

多重度

引数の多重度です。

デフォルト値

引数のデフォルト値の値指定を定義する式です。 デフォルト値は省略可能です。省略する場合

は、等号（=）も省略します。

プロパティ

'redefines' <oper-name> | 'query' | 'ordered' | 'unordered' | 'unique' | 'nonunique'
| 'seq' | 'sequence' | <oper-constraint>

redefines <oper-name>

オペレーションが <opername> で識別される継承されたオペレーションを再定義することを意

味します。

query

オペレーションがシステムの状態を変更しないことを意味します。

ordered

複数値の戻り引数がある場合に適用され、その値が順序付けられていることを意味します。

unordered

複数値の戻り引数がある場合に適用され、その値が順序付けられていないことを意味します。

unique

複数値の戻り引数がある場合に適用され、その値が重複していないことを意味します。

nonunique

複数値の戻り引数がある場合に適用され、その値が重複している可能性があることを意味しま

22 | ETロボコン技術教育資料

Rev. beb26.5.0

A.9 操作区画内と属性区画の記法

す。

seq または sequence

複数の値を持つ戻り引数がある場合に適用され、その値が順序付きバッグ （つまり、 isUnique =

false および isOrdered = true ） を構成することを意味します。

<oper-constraint>

操作に適用される制約です。引数リストは抑制される場合があります。

A.9.3 属性区画内の記法

属性

[<可視性>] [ '/' ] <属性名> [ ':' <型>] [ '['  <多重度>  ']' ] [ '=' <デフォルト値>]
[ '{' <プロパティに適用される修飾子> [ ','  <プロパティに適用される修飾子>]*  '}']

可視性

'+'  Public | '-'  Private | '#' Protected | '~' Package

/

属性が派生されていることを示します。

属性名

属性を言い表す名称とします。

型

属性のタイプを指定する式です。

多重度

属性の多重度です。

デフォルト値

属性のデフォルト値の値指定を定義する式です。 デフォルト値は省略可能です。省略する場合

は、等号（=）も省略します。

属性に適用される修飾子

属性に適用される修飾子(<prop-modifier>)を示します。

<prop-modifier> ::=  'readOnly' | 'union' | 'subsets' <property-name> | 'redefines'
<property-name> | 'ordered' | 'unordered' | 'unique' | 'nonunique' | 'seq' |
'sequence' | 'id' | <prop-constraint>

readOnly

属性が読み取り専用であることを意味します。

union

属性がそのサブセットの派生ユニオンであることを意味します。

subsets <property-name>

属性が <property-name> で識別される属性の適切なサブセットであることを意味しま

す。<property-name> は修飾される場合があります。

redefines <property-name>

属性が <property-name> で識別される継承されたプロパティを再定義することを意味しま

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 23

付録A クラス図とC++ プログラミング

す。<property-name> は修飾される場合があります。

ordered

属性が順序付けされていることを意味します。つまり、isOrdered = true です。

unordered

属性が順序付けされていないことを意味します。つまり、isOrdered = false です。

unique

複数値の属性に重複がないことを意味します。つまり、isUnique = true です。

nonunique

複数値の属性に重複がある可能性があることを意味します。つまり、isUnique = false です。

seq または sequence

属性が順序付きバッグを表すことを意味します。つまり、isUnique = false および isOrdered =

true です。

id

属性がクラスの識別子の一部であることを意味します。

<prop-constraint>

属性に適用される制約を指定する式です。

24 | ETロボコン技術教育資料

Rev. beb26.5.0

B.1 状態マシン図

状態マシン図とC++ プログラミング

付録B

B.1 状態マシン図

57. 状態マシン図はひとつのオブジェクトに着目して、その状態遷移を定義するモデルです。 状態マシン

図には、図 B.1振舞い状態マシン図とプロトコル状態マシン図の２つがあります。 ここでは、図 B.1振

舞い状態マシン図を取り上げます。 プロトコル状態マシン図については市販書籍をご参照ください。

STATE1

entry/ entry1
do/ do1
exit/ exit1

Trigger2 [guard2]

/ effect2

Trigger1 [ guard1] / effect1

STATE2

entry/ entry2
do/ do2
exit/ exit2

Trigger3 [ guard3] / effect3

図 B.1 振舞い状態マシン図

58. 状態の遷移はつぎのように記述します。

• イベント [ガード] / アクション　・・・　【UML1】
• トリガー [ガード] / エフェクト　・・・　【UML2】

イベント（event）【UML1】

状態が遷移するきっかけとなる振舞いです。

トリガー（trigger）【UML2】

遷移を可能にする事象を示します。トリガーの指定が複数可能です。この複数のトリガー

は、OR 条件 で、トリガーが発生します。

ガード（guard）

イベントが発生したときに評価されます。評価結果が真ならば遷移が有効です。偽ならば遷

移は無効です。なおガード式は副作用がない純粋な式です。省略可能です。

アクション（action）【UML1】

遷移が発生したときに実行されるアクションです。省略可能です。

エフェクト（effect）【UML2】

遷移が発火したときに実行されるアクションまたはアクティビティです。省略可能です。

「ガード」は省略可能ですが、省略しないように考慮してください。ただし、トリガーの違い

や状態の分け方によっては、ガードに頼らなくて済む設計のほうが好ましいときもあります。

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 25

付録B 状態マシン図とC++ プログラミング

B.1.1 状態の区画

名前区画

状態名を文字列で保持する

内部アクティビティ区画

振舞い状態マシンの「内部アクティビティ区画」に記載できるアクティビティの種類

entry

状態への入場の際に実行される挙動を示します。

do

モデル化された要素がその状態にある間、または指定された計算が完了するまで継続的に実

行される挙動を示します。この do ラベルは、モデル化された要素が状態にある限り、また

は式で指定された計算が完了するまで実行される進行中の行動（doActivity行動）を識別し

ます。

exit

状態からの退出の際に実行される挙動を示します。

他に内部遷移区画があります。内部遷移区画に上記「状態の遷移」を記述できます。なお、内

部遷移は自己遷移ではありませんので、entryとexitは実行されません。

B.1.2 状態マシン図をSwitch文で実装

59. つぎのプログラムは、上図の状態マシン図を C++プログラムのswitch文で実装したものです。 まず

前準備で色々な定義を行っています。 main関数で各状態のentryアクション、doアクション、exit

アクションを実行しています。 doアクションでトリガーを処理します。サンプルなのでキー入力で

トリガーを発生させています。実際は本来のトリガーを考慮するとよいでしょう。 遷移判定で期待す

るTRIGGERとguard条件が満足しているならば、effect処理とexit処理をして、つぎの状態に遷移し

ます。

リスト B.1 State_Switch.hpp

CPP

  1 #include <iostream>
  2
  3 // ── 状態の定義 ──
  4 enum class State {
  5     STATE1,
  6     STATE2,
  7     FINAL
  8 };
  9
 10 // ── イベント（トリガー）の定義 ──
 11 enum class Event {
 12     NONE,
 13     TRIGGER1,
 14     TRIGGER2,
 15     TRIGGER3
 16 };
 17
 18 // ── ガード条件 ──
 19 bool guard1() { return true; }
 20 bool guard2() { return true; }
 21 bool guard3() { return true; }

26 | ETロボコン技術教育資料

Rev. beb26.5.0

B.1 状態マシン図

 22
 23 // ── エフェクト ──
 24 void effect1() { std::cout << "→ effect1\n"; }
 25 void effect2() { std::cout << "→ effect2\n"; }
 26 void effect3() { std::cout << "→ effect3\n"; }
 27
 28 // ── トリガー読み取り ──
 29 Event readEvent() {
 30     std::cout << "Enter trigger (1→t1, 2→t2, 3→t3, 0→none): ";
 31     int i; std::cin >> i;
 32     switch (i) {
 33     case 1: return Event::TRIGGER1;
 34     case 2: return Event::TRIGGER2;
 35     case 3: return Event::TRIGGER3;
 36     default:return Event::NONE;
 37     }
 38 }
 39
 40 // ── 各状態の entry / do / exit アクション ──
 41 void entry1() { std::cout << "[Entry]  STATE1\n"; }
 42 Event do1() { std::cout << "[Do]     STATE1\n";  return readEvent(); }
 43 void exit1() { std::cout << "[Exit]   STATE1\n"; }
 44
 45 void entry2() { std::cout << "[Entry]  STATE2\n"; }
 46 Event do2() { std::cout << "[Do]     STATE2\n"; return readEvent(); }
 47 void exit2() { std::cout << "[Exit]   STATE2\n"; }
 48
 49
 50 int main() {
 51     Event evt = Event::NONE;
 52     State current = State::STATE1;
 53
 54     while (current != State::FINAL) {
 55         // ① entry アクション
 56         switch (current)
 57         {
 58         case State::STATE1:
 59             entry1();
 60             break;
 61         case State::STATE2:
 62             entry2();
 63             break;
 64         default:
 65             // 状態はSTATE1、STATE2だけしか定義していないのでここは通過しない
 66             break;
 67         }
 68
 69         int sameState = true;
 70         while (sameState) {
 71         // ② do アクション
 72             switch (current)
 73             {
 74             case State::STATE1:
 75                 // ③ イベントはdo1処理中に取得する
 76                 evt = do1();        ①
 77                 break;
 78             case State::STATE2:
 79                 // ③ イベントはdo2処理中に取得する
 80                 evt = do2();        ①
 81                 break;
 82             default:
 83                 //
    状態はSTATE1、STATE2だけしか定義していないのでここは通過しない
 84                 break;
 85             }
 86             // ④ 遷移判定と exit / effect / 次状態 entry

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 27

付録B 状態マシン図とC++ プログラミング

 87             switch (current) {
 88             case State::STATE1:
 89                 if (evt == Event::TRIGGER2 && guard2()) {
 90                     effect2();
 91                     exit1();
 92                     current = State::STATE2;
 93                     sameState = false;
 94                 }
 95                 break;
 96
 97             case State::STATE2:
 98                 if (evt == Event::TRIGGER1 && guard1()) {
 99                     effect1();
100                     exit2();
101                     current = State::STATE1;
102                     sameState = false;
103                 }
104                 else if (evt == Event::TRIGGER3 && guard3()) {
105                     effect3();
106                     exit2();
107                     current = State::FINAL;
108                     sameState = false;
109                 }
110                 break;
111
112             default:
113                 break;
114             }
115         }
116         std::cout << "-------------------\n";
117     }
118     std::cout << "== 終了状態に到達 ==\n";
119     return 0;
120 }

① Doアクティビティを実行中に当該イベントを検出するという処置が基本と考えます。当該イベン

トでない場合はDoアクティビティを実行し続けることも容易です。 Doアクティビティを実行し

た後であっても、Doアクティビティ内部の後処理でイベントを検出した方が良いと考えます。サ

ンプルでは、メッセージを表示して、イベント待ちとなっています。

B.1.3 状態マシン図をテーブル（配列）で実装

60. つぎのプログラムは、上記の状態マシン図を C++プログラムのテーブル（配列）で実装したもので

す。 状態マシンの各フェーズを配列と関数ポインタで制御しています。 まず前準備で色々な定義を

行っています。 main関数で各状態のentryアクション、doアクション、exitアクションを実行して

います。 doアクションで、サンプルなのでキー入力でイベントを発生させています。実際は本来の

イベントを考慮するとよいでしょう。 遷移判定で期待するTRIGGERとguard条件が満足しているな

らば、effect処理とexit処理をして、つぎの状態に遷移します。

リスト B.2 State_Array.hpp

  1 #include <iostream>
  2
  3 // ── 状態定義 ──
  4 enum State {
  5     STATE1 = 0,
  6     STATE2 = 1,
  7     FINAL = 2,
  8     STATE_COUNT = 3
  9 };

CPP

28 | ETロボコン技術教育資料

Rev. beb26.5.0

B.1 状態マシン図

 10
 11 // ── イベント定義 ──
 12 enum Event {
 13     NONE = 0,
 14     TRIGGER1 = 1,
 15     TRIGGER2 = 2,
 16     TRIGGER3 = 3
 17 };
 18
 19 // ── ガード条件 ──
 20 bool guard1() { return true; }
 21 bool guard2() { return true; }
 22 bool guard3() { return true; }
 23
 24 // ── エフェクト ──
 25 void effect1() { std::cout << "→ effect1\n"; }
 26 void effect2() { std::cout << "→ effect2\n"; }
 27 void effect3() { std::cout << "→ effect3\n"; }
 28
 29 // ── トリガー読み取り ──
 30 Event readEvent() {
 31     std::cout << "Enter trigger (1→t1, 2→t2, 3→t3, 0→none): ";
 32     int i; std::cin >> i;
 33     switch (i) {
 34     case 1: return TRIGGER1;
 35     case 2: return TRIGGER2;
 36     case 3: return TRIGGER3;
 37     default:return NONE;
 38     }
 39 }
 40
 41 // ── 各状態の entry/do/exit アクション ──
 42 void entry1() { std::cout << "[Entry]  STATE1\n"; }
 43 void entry2() { std::cout << "[Entry]  STATE2\n"; }
 44 void entryFinal() { /* nothing */ }
 45
 46 Event do1() {
 47     std::cout << "[Do]     STATE1\n";
 48     return readEvent();
 49 }
 50 Event do2() {
 51     std::cout << "[Do]     STATE2\n";
 52     return readEvent();
 53 }
 54 Event doFinal() { return NONE; }
 55
 56 void exit1() { std::cout << "[Exit]   STATE1\n"; }
 57 void exit2() { std::cout << "[Exit]   STATE2\n"; }
 58 void exitFinal() { /* nothing */ }
 59
 60 int main() {
 61     // 関数ポインタ型
 62     using EntryAction = void(*)();
 63     using DoAction = Event(*)();
 64     using ExitAction = void(*)();
 65
 66     // アクション配列
 67     EntryAction entryActions[STATE_COUNT] = { entry1, entry2, entryFinal };
 68     DoAction    doActions[STATE_COUNT] = { do1,    do2,    doFinal };
 69     ExitAction  exitActions[STATE_COUNT] = { exit1,  exit2,  exitFinal };
 70
 71     State current = STATE1;
 72
 73     while (current != FINAL) {
 74         // ── 直前の状態を記憶 ──
 75         State previousState = current;

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 29

付録B 状態マシン図とC++ プログラミング

 76
 77         // ① entry
 78         entryActions[current]();
 79
 80         int sameState = true;
 81         while (sameState) {
 82             // ② do → イベントはdo処理中に取得する
 83             Event evt = doActions[current]();   ①
 84
 85             // ③ 状態遷移（current を直接更新）
 86             if (current == STATE1) {
 87                 if (evt == TRIGGER2 && guard2()) {
 88                     effect2();
 89                     current = STATE2;
 90                     sameState = false;
 91                 }
 92             }
 93             else if (current == STATE2) {
 94                 if (evt == TRIGGER1 && guard1()) {
 95                     effect1();
 96                     current = STATE1;
 97                     sameState = false;
 98                 }
 99                 else if (evt == TRIGGER3 && guard3()) {
100                     effect3();
101                     current = FINAL;
102                     sameState = false;
103                 }
104             }
105         }
106         // ④ exit + 区切り表示
107         exitActions[previousState]();
108         std::cout << "-------------------\n";
109     }
110
111     std::cout << "== 終了状態に到達 ==\n";
112     return 0;
113 }

① Doアクティビティを実行中に当該イベントを検出するという処置が基本と考えます。当該イベン

トでない場合はDoアクティビティを実行し続けることも容易です。 Doアクティビティを実行し

た後であっても、Doアクティビティ内部の後処理でイベントを検出した方が良いと考えます。サ

ンプルでは、メッセージを表示して、イベント待ちとなっています。

B.2 Stateパターン

B.2.1 StateパターンのStateMachineクラス図

61. サンプルの状態マシン図のクラス図を 図 B.2 に示します。

62. StateMachineクラスで状態を実行管理します。

63. Stateクラスは基本クラスです。

ConcreteStateの2状態は、状態の具象クラスです。

64. Protectedメソッドとして、EntryState、DoState、ExitState を定義しています。これらは、Public

メソッドの handle から呼び出すことを想定しています。

30 | ETロボコン技術教育資料

Rev. beb26.5.0

B.3 StateパターンのStateMachine状態遷移図

図 B.2 StateMachineクラス図

B.3 StateパターンのStateMachine状態遷移図

65. 図 B.2 の状態マシン図が 図 B.3 になります。

66. ConcreteState1状態では、イベントAが発生した場合はConcreteState2へ遷移し、 イベントBが発生

した場合は自己遷移します。

67. ConcreteState2状態では、イベントCが発生した場合はConcreteState1へ遷移し、 イベントDが発

生した場合は終了状態へ遷移します。

図 B.3 StateMachine状態遷移図

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 31

付録B 状態マシン図とC++ プログラミング

B.3.1 C++プログラム

• Switch文、配列、ステートパターンの3パターンのプログラムのサンプルを示します。
• イベント入力はコンソールから行い、DoState 内で状態遷移が行われるようにしています。これは

テスト用ですから、本来のイベントを考慮するとよいでしょう。

• 実際のプロジェクトでは、動的メモリ管理の代わりにシングルトンや静的インスタンスを使うな

ど、効率・安全性の向上を検討してください。

B.3.2 StateMachineクラス図をSwitch文で実装

68. 具象クラスConcreteState1、ConcreteState2のhandle() の戻り値に基づいて状態遷移を実施してい

ます。

C++プログラム例

リスト B.3 stm_switch.hpp

CPP

  1 #include <iostream>
  2 using namespace std;
  3
  4 // 状態を表す列挙型
  5 enum StateID {
  6     STATE_CONCRETE_STATE1,
  7     STATE_CONCRETE_STATE2,
  8     STATE_TERMINATED
  9 };
 10
 11 class StateMachine; // 前方宣言
 12
 13 // 抽象クラス: State
 14 class State {
 15 public:
 16     // publicなhandle()の中で3フェーズを順次呼び出す
 17     StateID handle(StateMachine *context) {
 18         EntryState(context);
 19         StateID nextState = DoState(context);  // 次の状態を決定
 20         ExitState(context);
 21         return nextState;
 22     }
 23
 24     // デストラクタはpublicにする（delete時にアクセス可能にする）
 25     virtual ~State() {}
 26
 27 protected:
 28     // 状態の開始処理（各状態ごとに実装）
 29     virtual void EntryState(StateMachine *context) = 0;
 30     // 状態中の処理。イベント入力などで次の状態を決定して返す
 31     virtual StateID DoState(StateMachine *context) = 0;
 32     // 状態終了時の後処理（例：ログ出力）
 33     virtual void ExitState(StateMachine *context) = 0;
 34 };
 35
 36 // ConcreteState1 クラス
 37 class ConcreteState1 : public State {
 38 protected:
 39     void EntryState(StateMachine *context) override {
 40         cout << "\n【現在の状態：ConcreteState1】" << endl;
 41     }
 42
 43     // DoState で入力受付と次状態の決定を実施

32 | ETロボコン技術教育資料

Rev. beb26.5.0

B.3 StateパターンのStateMachine状態遷移図

 44     StateID DoState(StateMachine *context) override {
 45         cout << "イベントを入力してください (A: ConcreteState2 へ遷移, B:
    自己遷移): ";
 46         char eventChar;
 47         cin >> eventChar;
 48         switch (eventChar) {
 49             case 'A':
 50             case 'a':
 51                 cout << "イベントAが発生しました。ConcreteState2 へ遷移します。"
    << endl;
 52                 return STATE_CONCRETE_STATE2;
 53             case 'B':
 54             case 'b':
 55                 cout << "イベントBが発生しました。自己遷移します。" << endl;
 56                 return STATE_CONCRETE_STATE1;
 57             default:    ①
 58                 //
    便宜上、複雑なサンプルプログラムとしないように状態１でリターンさせています。
 59                 //
    皆さんのプログラムでは、defaultに対する適切な処理としてください。
 60                 cout << "不正な入力です。ConcreteState1 のままです。" << endl;
 61                 return STATE_CONCRETE_STATE1;
 62         }
 63     }
 64
 65     void ExitState(StateMachine *context) override {
 66         cout << "ConcreteState1 の Exit 処理を実行します。" << endl;
 67     }
 68 };
 69
 70 // ConcreteState2 クラス
 71 class ConcreteState2 : public State {
 72 protected:
 73     void EntryState(StateMachine *context) override {
 74         cout << "\n【現在の状態：ConcreteState2】" << endl;
 75     }
 76
 77     // DoStateで入力受付と次状態の決定を実施
 78     StateID DoState(StateMachine *context) override {
 79         cout << "イベントを入力してください (C: ConcreteState1 へ遷移, D:
    終了状態へ遷移): ";
 80         char eventChar;
 81         cin >> eventChar;
 82         switch (eventChar) {
 83             case 'C':
 84             case 'c':
 85                 cout << "イベントCが発生しました。ConcreteState1 へ遷移します。"
    << endl;
 86                 return STATE_CONCRETE_STATE1;
 87             case 'D':
 88             case 'd':
 89                 cout << "イベントDが発生しました。状態マシンを終了します。" <<
    endl;
 90                 return STATE_TERMINATED;
 91             default:    ①
 92                 //
    便宜上、複雑なサンプルプログラムとしないように状態１でリターンさせています。
 93                 //
    皆さんのプログラムでは、defaultに対する適切な処理としてください。
 94                 cout << "不正な入力です。ConcreteState2 のままです。" << endl;
 95                 return STATE_CONCRETE_STATE2;
 96         }
 97     }
 98
 99     void ExitState(StateMachine *context) override {
100         cout << "ConcreteState2 の Exit 処理を実行します。" << endl;

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 33

付録B 状態マシン図とC++ プログラミング

101     }
102 };
103
104 // StateMachine クラス
105 class StateMachine {
106 private:
107     State* currentState;
108
109 public:
110     StateMachine() {
111         // 初期状態は ConcreteState1
112         currentState = new ConcreteState1();
113         cout << "状態マシンを初期化しました。初期状態：ConcreteState1" << endl;
114     }
115
116     ~StateMachine() {
117         if (currentState != nullptr) {
118             delete currentState;
119         }
120     }
121
122     // 状態実行要求：handle() の戻り値に基づいて状態遷移を実施
123     void request() {
124         while (true) {
125             StateID nextState = currentState->handle(this);
126             delete currentState;
127
128             switch (nextState) {
129                 case STATE_CONCRETE_STATE1:
130                     currentState = new ConcreteState1();
131                     break;
132                 case STATE_CONCRETE_STATE2:
133                     currentState = new ConcreteState2();
134                     break;
135                 case STATE_TERMINATED:
136                     cout << "\n状態マシンを終了します。" << endl;
137                     currentState = nullptr;
138                     return;
139                 default:
140                     cout << "不明な状態です。終了します。" << endl;
141                     currentState = nullptr;
142                     return;
143             }
144         }
145     }
146 };
147
148 // エントリポイント
149 int main() {
150     StateMachine machine;
151     machine.request();
152     return 0;
153 }

① doアクティビティで状態が変わらない場合の処置のひとつは、 DoStateのなかで、当該イベント

でなければDoStateを続ける処理とすればよいです。 またはDoStateから抜けてきた場合

はhandleで

do{
    StateID nextState = DoState(context);
}while(nextState==STATE_SAME）；

69. として、Exit/Entry実行しないように処理すればよいです。

34 | ETロボコン技術教育資料

Rev. beb26.5.0

B.3 StateパターンのStateMachine状態遷移図

B.3.3 StateMachineクラス図をテーブル（配列）で実装

70. 単なる配列で状態インスタンスをStateMachine クラスで保持しています。配列なので、arrayクラ

スを活用してもよいでしょう。

C++プログラム例

リスト B.4 stm_table.hpp

CPP

  1 #include <iostream>
  2
  3 // イベントを表す列挙型
  4 enum class Event { A, B, C, D };
  5
  6 // 状態を表す定数を定義
  7 enum StateIndex { ConcreteState1Index, ConcreteState2Index };
  8
  9 // 前方宣言
 10 class StateMachine;
 11
 12 // 抽象基底クラス
 13 class State {
 14 public:
 15     virtual ~State() {}
 16     // 各状態の処理を実行し、イベントを返す
 17     virtual Event handle(StateMachine* context) = 0;
 18 protected:
 19     virtual void EntryState(StateMachine* context) = 0;
 20     virtual Event DoState(StateMachine* context) = 0;
 21     virtual void ExitState(StateMachine* context) = 0;
 22 };
 23
 24 // ConcreteState1クラス
 25 class ConcreteState1 : public State {
 26 public:
 27     virtual Event handle(StateMachine* context) override {
 28         EntryState(context);
 29         Event eventDoState = DoState(context);
 30         ExitState(context);
 31
 32         return  eventDoState;
 33
 34     }
 35 protected:
 36     virtual void EntryState(StateMachine* context) override {
 37         std::cout << "[ConcreteState1] EntryState\n";
 38     }
 39     virtual Event DoState(StateMachine* context) override {
 40         std::cout << "[ConcreteState1] DoState 処理\n";
 41
 42         int choice = 0;
 43         std::cout << "\n[ConcreteState1] イベント選択 (1=eventA, 2=eventB)：";
 44         std::cin >> choice;
 45         if (choice == 1) {
 46             std::cout << "  → Event A 発生：ConcreteState2 へ遷移\n";
 47             return Event::A;
 48         } else {
 49             std::cout << "  → Event B 発生：ConcreteState1（自己遷移）\n";
 50             return Event::B;
 51         }
 52     }
 53     virtual void ExitState(StateMachine* context) override {
 54         std::cout << "[ConcreteState1] ExitState\n";

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 35

付録B 状態マシン図とC++ プログラミング

 55     }
 56 };
 57
 58 // ConcreteState2クラス
 59 class ConcreteState2 : public State {
 60 public:
 61     virtual Event handle(StateMachine* context) override {
 62         EntryState(context);
 63         Event eventDoState = DoState(context);
 64         ExitState(context);
 65
 66         return  eventDoState;
 67
 68     }
 69 protected:
 70     virtual void EntryState(StateMachine* context) override {
 71         std::cout << "[ConcreteState2] EntryState\n";
 72     }
 73     virtual Event DoState(StateMachine* context) override {
 74         std::cout << "[ConcreteState2] DoState 処理\n";
 75
 76         int choice = 0;
 77         std::cout << "\n[ConcreteState2] イベント選択 (1=eventC, 2=eventD)：";
 78         std::cin >> choice;
 79         if (choice == 1) {
 80             std::cout << "  → Event C 発生：ConcreteState1 へ遷移\n";
 81             return Event::C;
 82         } else {
 83             std::cout << "  → Event D 発生：状態マシン終了\n";
 84             return Event::D;
 85         }
 86     }
 87     virtual void ExitState(StateMachine* context) override {
 88         std::cout << "[ConcreteState2] ExitState\n";
 89     }
 90 };
 91
 92 // StateMachine クラス
 93 class StateMachine {
 94 private:
 95     int currentStateIndex;  // ConcreteState1Index or ConcreteState2Index
 96     // 単なる配列で状態インスタンスを保持
 97     State* stateTable[2];
 98 public:
 99     StateMachine() {
100         stateTable[ConcreteState1Index] = new ConcreteState1();
101         stateTable[ConcreteState2Index] = new ConcreteState2();
102         currentStateIndex = ConcreteState1Index; // 初期状態は ConcreteState1
103         std::cout << "状態マシン開始：初期状態は ConcreteState1\n";
104     }
105     ~StateMachine() {
106         for (int i = 0; i < 2; ++i) {
107             delete stateTable[i];
108         }
109     }
110     // 状態の request() メソッド（イベントにより状態遷移）
111     void request() {
112         bool running = true;
113         while (running) {
114             Event event = stateTable[currentStateIndex]->handle(this);
115
116             // 状態判定を currentStateIndex で処理
117             if (currentStateIndex == ConcreteState1Index) {  // 現在が
    ConcreteState1
118                 if (event == Event::A) {
119                     currentStateIndex = ConcreteState2Index;  // ConcreteState1

36 | ETロボコン技術教育資料

Rev. beb26.5.0

B.4 StateMachineクラス図をステートパターンで実装

    → ConcreteState2
120                 } else if (event == Event::B) {
121                     currentStateIndex = ConcreteState1Index;  // ConcreteState1
    の自己遷移
122                 }
123             } else if (currentStateIndex == ConcreteState2Index) {  // 現在が
    ConcreteState2
124                 if (event == Event::C) {
125                     currentStateIndex = ConcreteState1Index;  // ConcreteState2
    → ConcreteState1
126                 } else if (event == Event::D) {
127                     running = false;        // 終了する
128                     std::cout << "状態マシン終了\n";
129                 }
130             }
131             std::cout << "---------------------------------\n";
132         }
133     }
134 };
135
136 int main() {
137     StateMachine machine;
138     machine.request();
139     return 0;
140 }

B.4 StateMachineクラス図をステートパターンで実装

71. ステートパターンは具象クラスの状態がつぎの状態を知っているという考え方です。

72. よって、つぎのコードのように当該状態が終了した時点でつぎの状態が設定されています。

   while (currentState != nullptr) {
        State* nextState = currentState->handle();  // 状態の実行
        delete currentState;                        // 状態の解放
        currentState = nextState;                   // つぎの状態の設定
   }

ストラテジパターンと混同しないようにしてください。

ステートパターンの詳細は、GoFや他の書籍を参照してください。

B.4.1 C++プログラム例

リスト B.5 stm_statepattern.hpp

  1 #include <iostream>
  2
  3 // 各イベントを表す列挙型
  4 enum class Event {
  5     A,  // ConcreteState1 で A を入力 → ConcreteState2 へ遷移
  6     B,  // ConcreteState1 で B を入力 → 自己遷移
  7     C,  // ConcreteState2 で C を入力 → ConcreteState1 へ遷移
  8     D,  // ConcreteState2 で D を入力 → 終了
  9     Invalid
 10 };

CPP

CPP

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 37

付録B 状態マシン図とC++ プログラミング

 11
 12 // 入力文字から Event を判断するヘルパー関数
 13 Event parseEvent(char c) {
 14     switch (c) {
 15         case 'A': return Event::A;
 16         case 'B': return Event::B;
 17         case 'C': return Event::C;
 18         case 'D': return Event::D;
 19         case 'a': return Event::A;
 20         case 'b': return Event::B;
 21         case 'c': return Event::C;
 22         case 'd': return Event::D;
 23         default:  return Event::Invalid;
 24     }
 25 }
 26
 27 // --- 抽象基底クラス State ---
 28 // 各状態クラスは、handle() 内で EntryState → DoState → ExitState を実行し、
 29 // 次の状態（または終了なら nullptr）を返します。
 30 class State {
 31 public:
 32     virtual ~State();
 33     virtual State* handle() = 0;
 34
 35 protected:
 36     virtual void EntryState() = 0;
 37     virtual Event DoState() = 0;
 38     virtual void ExitState() = 0;
 39 };
 40 State::~State() { }
 41
 42
 43 // forward 宣言（両方の具体状態を先に宣言）
 44 class ConcreteState1;
 45 class ConcreteState2;
 46
 47
 48 // --- ConcreteState1 の宣言 ---
 49 class ConcreteState1 : public State {
 50 public:
 51     ConcreteState1();
 52     virtual ~ConcreteState1();
 53
 54     // handle() の実装はクラス定義の外で行います
 55     State* handle() override;
 56
 57 protected:
 58     void EntryState() override;
 59     Event DoState() override;
 60     void ExitState() override;
 61 };
 62
 63
 64 // --- ConcreteState2 の宣言 ---
 65 class ConcreteState2 : public State {
 66 public:
 67     ConcreteState2();
 68     virtual ~ConcreteState2();
 69
 70     // handle() の実装はクラス定義の外で行います
 71     State* handle() override;
 72
 73 protected:
 74     void EntryState() override;
 75     Event DoState() override;
 76     void ExitState() override;

38 | ETロボコン技術教育資料

Rev. beb26.5.0

B.4 StateMachineクラス図をステートパターンで実装

 77 };
 78
 79
 80 // --- ConcreteState1 の実装 ---
 81 ConcreteState1::ConcreteState1() { }
 82 ConcreteState1::~ConcreteState1() { }
 83
 84 void ConcreteState1::EntryState() {
 85     std::cout << "[ConcreteState1] Entering state." << std::endl;
 86 }
 87
 88 Event ConcreteState1::DoState() {
 89     std::cout << "[ConcreteState1] Executing state action." << std::endl;
 90     std::cout << "Enter event (A: transition to ConcreteState2, B: self-
    transition): ";
 91     char input;
 92     std::cin >> input;
 93     return parseEvent(input);
 94 }
 95
 96 void ConcreteState1::ExitState() {
 97     std::cout << "[ConcreteState1] Exiting state." << std::endl;
 98 }
 99
100 State* ConcreteState1::handle() {
101     EntryState();
102     Event event = DoState();
103     ExitState();
104
105     switch (event) {
106         case Event::A:
107             std::cout << "[ConcreteState1] Event A received: Transitioning to
    ConcreteState2." << std::endl;
108             return new ConcreteState2();
109         case Event::B:
110             std::cout << "[ConcreteState1] Event B received: Self-transition in
    ConcreteState1." << std::endl;
111             return new ConcreteState1();
112         default:
113             std::cout << "[ConcreteState1] Invalid event: remaining in
    ConcreteState1." << std::endl;
114             return new ConcreteState1();
115     }
116 }
117
118
119 // --- ConcreteState2 の実装 ---
120 ConcreteState2::ConcreteState2() { }
121 ConcreteState2::~ConcreteState2() { }
122
123 void ConcreteState2::EntryState() {
124     std::cout << "[ConcreteState2] Entering state." << std::endl;
125 }
126
127 Event ConcreteState2::DoState() {
128     std::cout << "[ConcreteState2] Executing state action." << std::endl;
129     std::cout << "Enter event (C: transition to ConcreteState1, D: terminate):
    ";
130     char input;
131     std::cin >> input;
132     return parseEvent(input);
133 }
134
135 void ConcreteState2::ExitState() {
136     std::cout << "[ConcreteState2] Exiting state." << std::endl;
137 }

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 39

付録B 状態マシン図とC++ プログラミング

138
139 State* ConcreteState2::handle() {
140     EntryState();
141     Event event = DoState();
142     ExitState();
143
144     switch (event) {
145         case Event::C:
146             std::cout << "[ConcreteState2] Event C received: Transitioning to
    ConcreteState1." << std::endl;
147             return new ConcreteState1();
148         case Event::D:
149             std::cout << "[ConcreteState2] Event D received: Terminating
    StateMachine." << std::endl;
150             return nullptr;
151         default:
152             std::cout << "[ConcreteState2] Invalid event: remaining in
    ConcreteState2." << std::endl;
153             return new ConcreteState2();
154     }
155 }
156
157
158 // --- StateMachine クラス ---
159 // 内部で現在の状態の handle() を呼び出し、各状態が次の状態を返す設計です。
160 class StateMachine {
161 private:
162     State* currentState;
163
164 public:
165     // 初期状態（ConcreteState1）で開始
166     StateMachine(State* initialState) : currentState(initialState) { }
167
168     ~StateMachine() {
169         if (currentState) {
170             delete currentState;
171         }
172     }
173
174     // run() 内で状態遷移を処理します
175     void run() {
176         while (currentState != nullptr) {
177             State* nextState = currentState->handle();
178             delete currentState;
179             currentState = nextState;
180         }
181         std::cout << "StateMachine terminated." << std::endl;
182     }
183 };
184
185
186 //
187 // main() では StateMachine の起動（run() の呼び出し）のみ行います。
188 //
189 int main() {
190     StateMachine sm(new ConcreteState1());
191     sm.run();
192
193     return 0;
194 }

リスト B.5のプログラムから分かるように  循環依存の解決 が必要です。ConcreteState1 から

ConcreteState2 へのインスタンス生成や、その逆も両方可能となるようにします。典型的なステ

40 | ETロボコン技術教育資料

Rev. beb26.5.0

B.4 StateMachineクラス図をステートパターンで実装

ートパターンとして互いに遷移先を返す実装にします。

クラスの宣言と実装の分離 、リスト B.5の43行目のように、クラス宣言（プロトタイプ）のみを

行い、メンバ関数の実装はクラス定義の外で行います。

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 41

付録B 状態マシン図とC++ プログラミング

42 | ETロボコン技術教育資料

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録

ETロボコン技術教育資料

73. 資料名： 要素技術とモデルを開発に使おう 付録: ETロボコン技術教育資料

作成者： © 2016 – 2026 ETロボコン実行委員会

この文書は、ETロボコンの技術教育用のテキストです。

74. Rev. beb26.5.0, 2026-05-08 18:22:56 作成

Rev. beb26.5.0

要素技術とモデルを開発に使おう 付録 | 43


