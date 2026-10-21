# 公開最終 Graph_M／Graph_L 設計：需要整理的檔案清單

範圍：只公開**最終** Graph_M（D20 `<20,12>`、E_PAR=8、125 MHz）與 Graph_L
（D24 `<24,16>`、E_PAR=8、8.8 ns / 113.636 MHz，33 個 edge shard）的可建置程式碼。
目的：回答第 12 條（缺少 code／artifact availability statement），並支援第 32 條的
程式碼片段。

**不承諾的範圍：** 不宣稱「本 repo 可重現論文所有結果」。Fig. 7–10 與部分資源表來自原始
設計空間的多個配置（`5-evaluation.tex:65`，只有圖上的彙整值），不是靠最終兩份程式碼
就能重跑。CPU／GPU benchmark、Fmax sweep、全部 20 張圖、繪圖資料可日後另做
「實驗重現套件」，不是這次公開的前提。

## 1. 必放（最終設計公開版）

| # | 項目 | 內容 | 請確認 |
| --- | --- | --- | --- |
| 1 | Graph_M 完整可建置原碼 | `su_top_model.cpp` 及**實際依賴的所有 headers 與 .cpp**（`su_defines.h`、`su_util.cpp` 以及它們 include 的檔案）。不是只放 top 檔 | 用乾淨目錄做一次 HLS C-synthesis／C-sim，確認沒有漏檔 |
| 2 | Graph_L 完整可建置原碼 | 同上，`graph_l_d24/src/` 全部依賴 | 同上 |
| 3 | 設定 | 容量常數與 E_PAR、E2_PAR、NS_PAR、shard 大小（Graph_L：50,000 edges/shard、33 shards）、定點格式、時脈目標（125 MHz／8.8 ns） | 與論文 Table 2、Table 9 一致 |
| 4 | 權重 | 528 個參數（兩個設計共用或各自量化後的檔案），含格式說明 | 權重是否可公開；不能公開就在聲明寫明 |
| 5 | 至少一組合法輸入與預期輸出 | Graph_M 與 Graph_L 各一張圖（已排序、含 padding）與對應的預期輸出 | 輸入圖是否可公開（TrackML 衍生資料授權） |
| 6 | Testbench 與建置說明 | 可執行的 C-sim／co-sim testbench；Vitis HLS 專案 Tcl（`csynth`、`cosim` 的指令與參數） | 在乾淨環境照 README 實際跑過 |
| 7 | 版本與工具資訊 | 固定 commit／tag、Vitis HLS／Vivado 版本、目標元件（XCVU9P）、clock period 設定 | 能連到 8.8 ns 成功佈線的版本識別（build manifest、HLS export 或報告中的雜湊／日期） |
| 8 | README | 目錄結構、如何建置、如何跑 testbench、預期輸出、已知限制 | 見第 3 節 |
| 9 | LICENSE | 授權條款（MIT／Apache-2.0 等） | 先確認作者與所屬單位有權選擇授權；公開在 GitHub 不等於已給他人重用授權 |

## 2. 與論文片段的對應（第 32 條）

論文要引用的兩段程式碼必須在公開版中**逐字存在**，且行號以公開版為準：

| 片段 | 位置（audit 行號，需用公開版重新確認） |
| --- | --- |
| Graph_L Aggregator carry：`aggregation_global_par_8_cl_init`、尾端 carry 更新、首片與其後呼叫 | `su_top_model.cpp` L:1114、L:1470、L:6951、L:7021 |
| node table 的 URAM 複製與 lane 到副本的讀取映射（論文式：`i mod (E_PAR/2)`） | Graph_L L:6810 附近；Graph_M M:3899 附近 |

詳細問題見 `docs/SOURCE_CODE_REQUEST.md`。

## 3. README 必須寫清楚的限制

- 公開的是最終 Graph_M／Graph_L；Fig. 7–10 的設計空間配置、舊版設計、CPU／GPU
  benchmark、Fmax sweep 不在此 repo。
- FPGA 延遲是 RTL co-sim 的 cycle 數換算，不是上板量測；power 是 Vivado 的 vectorless
  估計（Medium confidence），不是板級量測。
- 若權重或測試圖不公開，註明哪些部分無法重跑，不要稱為「可完整執行」。
- 資源、時序、功率結果需要 Vivado 與授權，README 說明所用版本與約束，但不保證他人能
  得到完全相同的 P&R 結果（run-to-run 變異未評估）。

## 4. 公開前的清理

- 移除 `<local-workspace>/` 等絕對路徑、內部帳號、機器名稱。
- 移除暫存檔、DCP、大型 log、未使用的舊版檔案；保留的舊檔要標明不對應論文。
- 舊的 Graph_L 1.086 ms 設計不得當成結果放入。
- 不放：前人論文 PDF（Huang、Elabd）、回覆信草稿、`experiment_data/` 內部報告。
- 檢查程式碼註解沒有未公開的內部資訊。

## 5. Code availability 聲明草稿方向（待 repo 公開後再填網址）

聲明要準確寫出：公開了什麼（最終 Graph_M／Graph_L 的 HLS 原碼、設定、測試輸入／權重、
testbench 與建置說明）、沒公開什麼（設計空間研究的其他配置、CPU／GPU benchmark 腳本、
實驗原始資料，或依授權限制的輸入圖）、固定版本（commit／tag，或 Zenodo DOI）與授權。
在 repo 真的公開前，只能寫 "will be released at …"，不能先填網址。

## 6. 之後的流程

1. 你整理上述檔案，並回答 `docs/SOURCE_CODE_REQUEST.md` 的問題。
2. 我先列出候選片段與行號讓你確認，不修改論文。
3. 你同意後，我才新增 Code availability 一節、加入程式碼片段，更新追蹤表第 12、32 條與
   `revision/revision_record.md`，並重新建置。
4. `[O]` 狀態需你明確指示才標記。
