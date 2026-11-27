# BGM(M63 #25、親 TM-59/project-context#598)

方針(project-context 20-stacks.md「ゲームの BGM」): 基調はバラード。戦闘の役と物語は別の曲で、場面の切り替えで替える。歌は文字を読まない場面だけ。

## 鳴らす経路

ブラウザー版(C の時空図・ブラックホール・窓の眺めを Emscripten で動かす頁)だけに Web Audio の再生を足した。物理は C のまま、JS は画面から曲を選ぶだけ。
端末版・CSV/SVG/GIF は無音のまま。曲は最初の操作の後に `bgm/list.json` と `bgm/<曲>.ogg` を読む(初期ロードに入れない)。読めない・復号できない曲は無音のまま遊びを止めない。

## 場面 → 曲

| 画面 | 曲 | 強さ |
|---|---|---|
| 時空図の手動飛行・1 g flip-and-burn の実演・Orbit a black hole・The view from the window | drift(基調のバラード) | 1 |
| The Long Echo の区間(leg)を飛ぶ間 | leg(戦闘の役、pulse) | 1(一時停止・噴射前の待ちは 0.4) |
| 区間の場面(pad・gate・depot) | story | 0.7 |
| 重い場面(dust・star・silence) | story | 0.4 |
| Home の場面(結末)と 5 区間を終えた後 | home | 1 |

戦う相手・主戦は無い。燃料と時計で往復の区間を飛ぶ間を戦闘の役にした。場面を閉じた後は噴射を押すまで一時停止なので、その間も 0.4 倍。手動飛行と実演は押すまで止まっているのが普通なので一時停止で下げない。場面は選択を押すまで止まる(時間で流れない)。判定は `web/bgm.mjs` の `cue()`(物語か・場面・重さ・結末か・一時停止・終わったかだけを読む)。

- 切り替えは 2.5 秒の交差フェード。一時停止は 0.4 倍。
- **全部器楽**: どの場面も文字か計器を読むので、歌入りは作らない(英日の物語の文も器楽の下で読む)。
- 消音: 見出しの ♪(44px)と `M`(どちらも新設。M は他の操作に使われていない)。保存はしない(この repo は保存機能を持たない)。消音は出口(master)でも切るので、交差フェードで消えていく前の曲も同時に黙る(TM-59/project-context#668 の穴を最初から作らない)。

## 曲(採用・門の値)

| 場面(曲) | 鳴る画面 | 門 | 採用 | 通過 | ループ | LUFS / 山 | 継ぎ目の音色 | 落ち込み / 静かな窓 | 大きさ |
|---|---|---|---|---|---|---|---|---|---|
| drift | 基調(手動飛行・実演・ブラックホール・窓の眺め) | soft | drift-c1-s944 | 11/12 | 27.097 秒 | -16.0 / -1.7 dBFS | 0.63 | 1.0 秒 / 0.056 | 603,104 B |
| leg | 戦闘の役(The Long Echo の 5 区間を飛ぶ間) | pulse 57.7 BPM(計画 116、半分の拍) | leg-c1-s943 | 16/18 | 33.037 秒 | -16.0 / -1.2 dBFS | 0.53 | 0.0 秒 / 0.0 | 832,348 B |
| story | 物語(区間の場面。重い場面 dust・star・silence と一時停止は 0.4 倍) | soft | story-c1-s941 | 3/12 | 21.429 秒 | -15.9 / -1.7 dBFS | 0.74 | 1.5 秒 / 0.19 | 441,128 B |
| home | 結末(Home の場面と終えた後) | soft | home-c0-s942 | 6/12 | 16.0 秒 | -16.0 / -3.9 dBFS | 0.49 | 0.0 秒 / 0.0 | 384,742 B |

計 2,261,322 B(4 曲)。通過 36/54。全候補の値は `docs/bgm/gate.md`、生の値は `docs/bgm/report.json`。

## 作り方

1. 計画: `docs/bgm/plan.json`(場面 × caption × seed 6、seed 941〜946。pulse の leg だけ caption 3 つで、どれも頭から拍が続く指示。他の作品と同じ caption・seed を使わない)。caption は音の特徴だけで、曲名・作者・作品名を書かない。
2. 生成: ACE-Step 1.5 turbo(MIT、steps 8)を Colab の L4 で 1 回の確保(54 本、端末からの 1 回 367.3 秒、計算枠 587.33 → 587.33。0.01 未満)。
   `cd ~/dev/local-llm-ops && .venv/bin/python -m tools.colab bgm --repo <この repo> --out <dir> --backend colab --gpu L4`
   Colab が使えない時の落ちる先は `scripts/bgm_gen.py`(手元の CPU、local-llm-ops の music venv)。
3. 門と書き出し: `~/.local/share/local-llm-ops/genvenv/music/bin/python scripts/bgm_bake.py --takes <dir>`(ffmpeg が要る)。
   門は gravity-well #35・spot-gap #33・td-games #38 と同じ値で緩めていない: soft(バラード・物語・結末)は local-llm-ops `tools/genmodels/mood.py` の soft の門、pulse(戦闘の役)は PULSE_GATE + テンポ ±5%、全部に -16 LUFS ±1.5・山 -1 dBFS・継ぎ目の音色の跳び 2 倍以下・12 dB の落ち込み 2 秒以下、pulse は静かな窓の割合 0.10 以下、soft のループは 16 秒以上。
   `scripts/bgm_bake.py`・`scripts/bgm_gen.py` は gravity-well #35 の物と同じ(soundfile・matplotlib を使わず ffmpeg で読み書き・作図)。名前は local-llm-ops の `tools.colab bgm` が `scripts/bgm_gen.py` を探すので下線のまま(この repo の他の scripts はハイフン)。
4. 出所と利用条件: 曲は ACE-Step 1.5(MIT、出力に第三者の権利の主張なし)で手元の計画から生成した独自の音源。第三者の音源・歌詞・旋律は使っていない。外部の生成 AI・API key は使っていない。

## 波形とスペクトル(自分で見た所見)

画像は 2 周つないだ物で、真ん中が継ぎ目(上が波形、下が対数の周波数のスペクトル)。

- `drift`: 鍵盤の句がまとまって鳴り、句の間は薄い持続音(落ち込み 1.0 秒)。継ぎ目(0.63)の段差は見えない。
- `leg`: 頭から終わりまで等間隔の拍が詰まって続き、薄い区間が無い(落ち込み 0・静かな窓 0)。拍の検出は 57.7 BPM(計画 116 の半分。門はテンポの倍・半分を同じ拍と数える)。
- `story`: 句ごとに鍵盤と弦が鳴り、句の間は静か(最長 1.5 秒、静かな窓 0.19)。台詞の邪魔をしない薄さ。12 本中 3 本だけ通過(落ちたのは音色の跳び・局所の調)。
- `home`: 旋律と弦の帯が全体に続き、短い静けさを挟む。山 -3.9 dBFS。継ぎ目の段差は見えない(0.49)。

人の試聴は門にしない(客観の門を通した物だけを採る)。

## 試験

- `node scripts/bgm-smoke.cjs`(ローカル専用、port `BGM_PORT` 既定 22092、記録 `docs/bgm/smoke.json`):
  最初の操作の前は何も読まない(0 件)・drift(出口 -21.1 dBFS)・pad の場面で story(-30.3、0.7 倍)・交差フェード中の M で 0.8 秒後 -120 dBFS・leg(-24.8)・一時停止で利得 0.45 → 0.18(-34.5)・♪ 44×44 で -120 → -34.6・重い場面と Home の cue・dist 2,646,775 B(10 MB の門の内。BGM 4 曲 2,261,322 B を含む。main は 376,305 B)。
- 既存の `make test`(解析解・保存則・adapter・物語の `story_bench --check` 8 行一致)と `make smoke`(156 項目、390×844・844×390 の文字と 44px を含む)は通る。撮り直した画像と docs/validation.json は戻した。
  ♪ を足した最初の版は 390×844 で ♪ が flex に縮められて 29.7px になり、見出しも 401px に広がって smoke が落ちたので、♪ を縮ませず、幅 720px 以下で飾りの矢印を隠して間を詰めた。
- 遊べる門(`bin/playability-gate.py`、main と branch を同じ時間帯 2026-11-27 10:06):1 回目 main 4/7・branch 3/7(branch の横の ②タップで進行が 25 秒で未到達)、続けての 2 回目 main 4/7・branch 4/7(② p 21.3 / l 17.1 秒が同じ値)。落ちている ④文字・⑤タップ・⑥縦横は main と同じ既知の型。
