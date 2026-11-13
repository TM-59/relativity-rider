# Relativity Rider

C11 + libm の 1+1 次元特殊相対論シミュレータ。物理の正典は `src/relativity.c`(一定の固有加速度の双曲運動を閉形式で進める)。
端末版・CSV/SVG/GIF と、同じ C 計算を WASM で使うブラウザーの時空図観測画面(`src/rider.c` + `web/`)を持つ。

- 単位は c = 1(年・光年)。1 g = 1.0323 ly/yr²。目的地は 4.2 ly、到着は 0.05 ly 以内かつ 0.05 c 未満(端末版と同じ)。
- ブラウザーの 1 ステップは固有時 half_tau / 400 ≈ 0.0044 yr。JavaScript は描画と入力だけで、位置・時計・速度・世界線は C で計算する。
  時空図の「ロケットの今」の線(t − t0 = v (x − x0))だけは C の v から JS で引く幾何の補助線。
- 原画の背景と 2 人の狐(ロケットの Vela・地球に残る Oren)は装飾。肖像の目印は C が計算した事象(現在の (x, t) と、地球系で同時刻の地球)に置くだけ(画面と README に明記)。
- 乱数・保存・サーバー API は無い。既存の出力(CSV/SVG/GIF)と物理モデルを保持する。
- 検証は `make test`(解析解・保存則の照合 + adapter)/ `make bench`(native trace)/ `make plots` / `make web` / `make smoke`。
  native の cc が無い端末では、gcc の入ったコンテナで `make -j4 test bench relativity-rider` を実行して native を作り
  (例: `docker run --rm -u $(id -u):$(id -g) -v $PWD:/src -w /src <gcc の入ったイメージ> make -j4 test bench relativity-rider`)、
  Emscripten を有効にして `make web && node scripts/smoke.cjs` を手元で回す。
- 理論の手引きは `docs/theory/`。`make theory-figs` が図と実測値の表を `tests/theory_figs.c` から再生成し、`make theory-check` が
  対応表(文 → 頁 → 図 → 関数 → テスト)の欠け・リンク切れ・消えた関数名・未掲載の公開関数で落ちる。法則・規則を足したら同じ PR で頁と図と表を足す。
  README の「30 statements」は表の行数と合わせる。
- スマホ(幅 720px 以下・高さ 500px 以下・タッチ主体)では DOM と canvas の全文字を 14 CSS px 以上、操作を 44 CSS px 以上にし、噴射ボタンは時空図の横に固定する。`make smoke` が 390×844 と 844×390 で実測する。`window.__tmGate` は読み取り専用。
- CI は PR の軽量ゲート(docker build で `make test && make plots`)のみ。WASM ビルド・撮影・E2E はローカル専用。
- 公開 repo。コミット・PR に生成 AI の署名、私的な情報を入れない(公開前に private 情報の検査と差分の目視を行う)。
- 実画面は `docs/images/`(`make smoke` が撮る)、原画と生成指示・採否は `assets/art/` と `docs/art-direction.md`。

## M2 のブラックホール軌道(一般相対論)

- `src/schwarzschild.c` が Schwarzschild 時空の測地線(質点・光、比較用のニュートン)を幾何単位 G = c = M = 1 で積分する(Yoshida の 4 次シンプレクティック合成、地平面 2M(1 + 1e-3) で停止)。特殊相対論のモデル `src/relativity.c` とその出力・物語は変えない。
- `src/orbit.c` がブラウザーの「Orbit a black hole」と端末の `--orbit`(docs/orbit.svg)の adapter。JS(`web/orbits.mjs`)は描画と入力だけ。
- `tests/test_schwarzschild.c` が閉形式(円軌道・ISCO 6M・光子球 3M・b_c・楕円積分の近日点移動・4M/b・自由落下)と収束次数を照合し、`--trace` を smoke が WASM と比べる。理論は `docs/theory/05-schwarzschild.md`(S19–S30)。

## M5 の物語の層「The Long Echo」

- `src/courier.c` が物語の往復(地球 → ゲート 0.8 → 補給所 2.0 → ケストレル 4.2 → 中間ブイ 2.0 → 地球)。ロケットは `rocket_step` だけで動かす(噴射 ±1 g・制動・慣性)。
  物語の数値(燃料・待機・表示の揺れ・漏れ・救助)は `CE_*` のビットで外せる。場面の文は `web/story.mjs`(英日)。
- `make story` が docs/narrative/{play-order,ablation,tension}.json を書き、`make test` が行を再計算して一字一句の一致と種の重複なしを強制する。
  数値を変えたら `make story` で書き直し、`tests/test_courier.c` の `[mech]`・`[beat]`・`[choice]` を通す。CI(Debian 13)でもバイト一致すること。
- 重い場面(塵・ケストレル・沈黙)は暗い静止パネルで、到着の帯を出さない(`[quiet]` とスモーク)。

## M4 の独自原画の例外

ローカルの Stable Diffusion 1.5(stable-diffusion.cpp・CPU、API key なし)で生成した独自の原画
(宇宙の背景・2 人の狐の肖像)を許可する。時空図の背景・時計カード・世界線上の目印に実描画する。
既存作品の名称・意匠、実在の人物、第三者の画像・外部音声・外部フォントは持ち込まない。原画を含め配布 10,000,000 B 以下を守る。
