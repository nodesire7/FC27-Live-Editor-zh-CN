# DLL 显示键深度审计（2026-11-18）

从字段构造器 0x30bc90 的 341 处调用中核对 268 个确定名称，球员构造函数 0x37be00–0x38300a 包含 152 个确定字段。

根因：字段刷新器从对象 +0x20 的原始字段名生成 `_abbr` 与 `_tooltip`，再调用 0x20e5d0。基础键存在不代表派生显示键存在。球员刷新函数 0x383010 中，0x383135 引用 `_abbr`，0x383146/0x383151 拼接，0x38316a 查本地化，结果存至字段 +0x60；0x38321a 走 tooltip 同类路径。

缺失映射 306 条（含确认的设置 tooltip）。完整、可合并映射在 JSON 的 missing_mappings。all_confirmed_derived_labels 是证据支持的全部字段派生键；不要覆盖已有更详细 tooltip。

## 球员编辑器缺失短标签

- `birth_day_abbr` → 出生日期（日）；构造调用 0x30fa68, 0x37d082
- `birth_month_abbr` → 出生月份；构造调用 0x30fb5d, 0x37d117
- `birth_year_abbr` → 出生年份；构造调用 0x30fc3a, 0x37d1af
- `join_team_day_abbr` → 入队日期（日）；构造调用 0x30fd92, 0x37f608
- `join_team_month_abbr` → 入队月份；构造调用 0x30fe9b, 0x37f69d
- `join_team_year_abbr` → 入队年份；构造调用 0x30ff86, 0x37f735
- `nt_jerseynumber_abbr` → 国家队球衣号码；构造调用 0x37c8fd
- `fm_fullfit_d_abbr` → 完全恢复（日）；构造调用 0x37d842
- `fm_fullfit_m_abbr` → 完全恢复（月）；构造调用 0x37d8d7
- `fm_fullfit_y_abbr` → 完全恢复（年）；构造调用 0x37d96f
- `selection_bias_pos_abbr` → 入选倾向适用位置；构造调用 0x382956
- `is_transfer_listed_abbr` → 挂牌转会；构造调用 0x382ce6
- `is_loan_listed_abbr` → 挂牌租借；构造调用 0x382d7b

## 准确性修正

- `selection_bias_pos`：入选倾向适用位置；注册类型为 `positions_sel_bias`。
- `nt_jerseynumber`：国家队球衣号码。
- `jersey_name_color` / `jersey_name_outline_color`：球衣姓名颜色 / 姓名描边颜色；现基础键误称号码，而现有 abbr 已正确。

## 硬编码显示与排除项

- 0x468165 编辑/复制提示直接送到 0x27c160，当前指针补丁没有覆盖。
- 0x43df09 球员网页提示先插入 URL 再本地化，向字典增加模板本身无效；需要显示指针及显式长度补丁。
- 0x3eeadd 进球数模板直接格式化到描述符标签；需要指针和长度补丁。
- 三个看似缺失的英文（CPU AI Long Shot Taker、Preferred Position 1 - Main、GK Primary Attributes Boost: bbbb）仅计算文本宽度；不能当作可见遗漏。
- 4 处非字面量构造另列 excluded_dynamic_constructor_calls，未伪造数字或坐标键。

本报告 tooltip 映射使用保守字段名称；只保证不会暴露英文参数名，未编造具体游戏机制。未修改主生成器、DLL、部署文件或 GitHub。

补充：经理页面模板 0x4458f8（显式长度 0x445903 = 38）和球队页面模板 0x452e9a（长度 0x452ea6 = 37）也确认先格式化再本地化，同球员页面处理，共 5 处硬编码显示遗漏。
