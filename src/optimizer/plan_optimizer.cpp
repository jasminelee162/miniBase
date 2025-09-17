#include "plan_optimizer.h"
#include "../util/logger.h"
#include <algorithm>

namespace minidb {

static std::vector<std::string> splitConjuncts(const std::string &pred) {
    // 极简实现：按 " AND " 分割；未来可替换成真正的表达式解析
    std::vector<std::string> out;
    if (pred.empty()) return out;
    size_t start = 0;
    while (true) {
        size_t pos = pred.find(" AND ", start);
        if (pos == std::string::npos) { out.push_back(pred.substr(start)); break; }
        out.push_back(pred.substr(start, pos - start));
        start = pos + 5;
    }
    // 去掉空白项
    out.erase(std::remove_if(out.begin(), out.end(), [](const std::string &s){ return s.empty(); }), out.end());
    return out;
}

static std::string mergeConjuncts(const std::vector<std::string> &conjs) {
    std::string res;
    for (const auto &c : conjs) {
        if (c.empty()) continue;
        if (!res.empty()) res += " AND ";
        res += c;
    }
    return res;
}

static bool looksPushableToScan(const std::string &c) {
    // 极简启发：包含比较运算符并且左侧像列名、右侧像常量
    // 真正实现应基于表达式树/语义信息
    return (c.find('=') != std::string::npos) || (c.find('<') != std::string::npos) || (c.find('>') != std::string::npos);
}

static std::unique_ptr<PlanNode> rewrite(std::unique_ptr<PlanNode> node) {
    if (!node) return node;
    for (auto &ch : node->children) {
        ch = rewrite(std::move(ch));
    }

    if (node->type == PlanType::Filter && !node->children.empty()) {
        auto &child = node->children.front();
        if (!child) return node;

        // Filter over Project: 尝试下推
        if (child->type == PlanType::Project && !child->children.empty()) {
            // 先取出孙节点
            auto grand = std::move(child->children.front());
            child->children.clear();

            // 构造一个新的 Filter 节点（复制必需属性），避免对 node 的不安全移动
            auto newFilter = std::make_unique<PlanNode>();
            newFilter->type = PlanType::Filter;
            newFilter->table_name = node->table_name;
            newFilter->predicate = node->predicate;
            newFilter->children.push_back(std::move(grand));

            // 将新 Filter 作为 Project 的子节点，并递归重写
            child->children.push_back(rewrite(std::move(newFilter)));
            return std::move(child);
        }

        // Filter over SeqScan: 拆分合取项，下推可下推部分
        if (child->type == PlanType::SeqScan) {
            std::vector<std::string> parts = splitConjuncts(node->predicate);
            std::vector<std::string> toScan, toRemain;
            for (auto &p : parts) {
                if (looksPushableToScan(p)) toScan.push_back(p); else toRemain.push_back(p);
            }
            if (!toScan.empty()) {
                std::string merged = mergeConjuncts({child->predicate, mergeConjuncts(toScan)});
                child->predicate = merged;
            }
            if (toRemain.empty()) {
                return std::move(child); // Filter 消除
            } else {
                node->predicate = mergeConjuncts(toRemain);
                return node;
            }
        }
    }

    // 投影剪枝占位：暂不变更 schema，仅为将来扩展保留结构
    return node;
}

std::unique_ptr<PlanNode> OptimizePlan(std::unique_ptr<PlanNode> plan) {
    Logger logger("logs/planner.log");
    logger.log("[Optimizer] run lightweight rules: predicate pushdown / projection placeholder");
    return rewrite(std::move(plan));
}

} // namespace minidb


