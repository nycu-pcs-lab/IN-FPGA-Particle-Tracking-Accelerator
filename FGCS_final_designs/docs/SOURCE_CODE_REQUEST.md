# 需要整理給 Claude 的 HLS 原始碼（Item 12／32 的程式碼片段）

目的：在論文 §4 加入 1–2 段短的 HLS 原碼片段，並在 Code availability 公開 repo。
片段必須**逐字取自最終版原始碼**；我不憑記憶或 audit 摘錄改寫。

最終版 = 論文數值對應的版本：Graph_M（D20 `<20,12>`、E_PAR=8、125 MHz）與
Graph_L（D24 `<24,16>`、E_PAR=8、8.8 ns / 113.636 MHz，33 個 edge shard）。

## 1. 先請提供的資訊

| 項目 | 說明 |
| --- | --- |
| 原始碼根目錄在本機的路徑 | `graph_l_d24/src/` 與 Graph_M 對應目錄；目前工作目錄找不到 |
| 版本 | git commit hash 或 tag（若尚無版本控制，請說明檔案日期），需與 8.8 ns / D24 / D20 最終結果一致；如何證明一致見下一列 |
| 前處理／testbench | 建圖與排序（sorted by destination）、shard 切分與 padding 的程式或腳本，路徑與版本同上 |
| 檔案 | `su_top_model.cpp`、`su_defines.h`、`su_util.cpp`（`su_host_load.h` 不需要） |
| 版本識別 | `su_top_model.cpp` 本身不能證明「這是 8.8 ns 成功佈線的版本」。除 commit／tag 外，請提供能連到該次結果的紀錄：build manifest、HLS export 或 Vivado Tcl、或報告（如 `graph_l_d24_8p8ns_resource_latency_power_report.md`）中的版本識別（檔案雜湊、日期、run 名稱） |
| 行號基準 | `MODEL_DATAPATH_SOURCE_CHECK.md` 的 `L:`／`M:` 行號是 audit 讀到的行號；請確認它們對應上述版本，否則以函式名稱為準 |

## 2. 片段 1（首選）：Graph_L 跨 shard 的 Aggregator carry

用途：說明大圖分片後，未完成的目的節點累加如何跨 shard 保留與接續，邊界上不遺失訊息。

請提供下列**完整函式與呼叫處**（不要只截 static 變數）：

| 內容 | 位置（audit 行號，需重新確認） |
| --- | --- |
| 函式 `aggregation_global_par_8_cl_init` 全文 | `su_top_model.cpp` L:1114 起 |
| 函式尾端更新 carry（`keep_e_pre`、`keep_idx_pre`、`keep_n_pre`）的部分 | L:1470 附近 |
| 第一片呼叫（`init = true`） | L:6951 |
| 其後各片呼叫（`init = false`） | L:7021 |
| `sens_agg_edge`、`index_t`、`node`、`MAX_NODE`、`EDGE_DIM` 的型別／常數定義 | `su_defines.h` |

需要同時回答：
1. `keep_n_pre` 沒有顯式初始化（audit 已指出）；它第一片時的有效性依賴什麼（初始 dummy token？）
2. 這個 carry 是否只在 Graph_L 使用，Graph_M 的 aggregator 是不是另一個函式？
3. carry 的正確性是否依賴目的節點 ID 依序到達（sorted by destination）？HLS aggregator 只能顯示它依賴這個前提，不能證明輸入一定排序。請另外提供輸入端的證據：建圖／前處理程式或 testbench 如何保證排序，以及 shard 邊界與 padding 的處理（例如 shard 切在同一目的節點中間時 carry 如何接續、padding 邊的目的 ID 是什麼）。

## 3. 片段 2（篇幅允許時）：node table 的 URAM 複製與讀取映射

用途：說明 E_PAR/2 份複製下，edge lane 如何對應到副本，與 §4.4 的描述一致。
重點是**讀取映射**，不是 `BIND_STORAGE`／`ARRAY_PARTITION` 指令本身（那是標準 HLS 用法，不稱為新穎性）。

請提供：

| 內容 | 位置（audit 行號，需重新確認） |
| --- | --- |
| `node_src_mp_L1`、`node_des_mp_L1` 的宣告與 `BIND_STORAGE`／`ARRAY_PARTITION` pragma | Graph_L：L:6810 附近；Graph_M：M:3899 附近 |
| EdgeBlock1 中 lane `i` 讀取副本的程式碼與實際索引式（論文為 `i mod (E_PAR/2)`） | `edge_block_*` 相關函式，請指出函式名稱 |
| 陣列維度與副本數的關係（`[4][43278]` 中 4 = E_PAR/2） | 同上 |

需要同時回答：
1. 論文 §4.4 寫的是 lane $i$ 讀 copy $i \bmod (E_{\mathrm{PAR}}/2)$（`4-architecture.tex:105`）。$E_{\mathrm{PAR}}=8$ 時 lane 0／4 共用 copy 0、lane 1／5 共用 copy 1，依此類推（不是 `i/2`，那會讓 lane 0／1 共用 copy 0，兩者不同）。請從原始碼確認實際索引式；若原碼不同，**先修正論文**，再選片段。
2. 每個副本是 dual-port，兩個 lane 共用一份，這點在程式碼中如何體現？

## 4. 不需要的部分

- 頂層 `DATAFLOW` 串接：架構圖與 Listing 1 已涵蓋。
- 單獨的 `PIPELINE II=1`：標準用法，不展示設計貢獻。
- `BIND_OP`（opt_rsrc 把乘法移到 LUT）：正文一句話帶過即可。
- 先前移除的 §7.2 稀疏表合併 `merge_step` 摘錄：除非你認為它比片段 2 更重要，否則不放。

## 5. 整理格式建議

每個片段請附：
1. 檔名、函式名、起訖行號（以實際檔案為準）。
2. 逐字貼上的原始碼（保留原註解與縮排）。
3. 你認為可以用 `...` 省略的部分（標明哪幾行）。
4. 上面「需要同時回答」的問題答案。

## 6. 公開 repo（Code availability，Item 12）需要確認的事

| 問題 | 為什麼需要 |
| --- | --- |
| GitHub 網址與授權條款（MIT／Apache-2.0 等） | Code availability 一節要寫 |
| 是否用 Zenodo 取得固定版本 DOI | 論文引用固定版本 |
| 公開哪些：HLS 原始碼、Graph_M／Graph_L 設定、co-sim testbench、pseudo-graph 產生腳本、`fgcs_in_benchmark`、Fmax／P&R Tcl 腳本 | 決定聲明文字 |
| 輸入圖（TrackML 衍生）能否公開 | 不能時改寫為「提供產生圖的腳本」 |
| 預計公開日期 | 公開前只能寫 "will be released at ..."，網址不能先填 |

## 7. 之後的流程

1. 你提供上述內容。
2. 我先列出候選片段與行號，讓你確認（不修改論文）。
3. 你同意後，我才寫進 `revision/4-architecture.tex`，新增 Code availability 一節，更新追蹤表第 12、32 條備註與 `revision/revision_record.md`，並重新建置。
4. 狀態 `[O]` 仍需你明確指示才會標記。
