module;

#include <algorithm>
#include <string>

export module paclook.util;

import paclook.package;

export namespace paclook {

// Sort packages by relevance to query
// Priority: exact match > starts with > contains in name > description only
void sort_by_relevance(PackageList& packages, const std::string& query) {
    std::string query_lower = query;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);

    std::sort(packages.begin(), packages.end(),
        [&query_lower](const Package& a, const Package& b) {
            std::string a_lower = a.name;
            std::string b_lower = b.name;
            std::transform(a_lower.begin(), a_lower.end(), a_lower.begin(), ::tolower);
            std::transform(b_lower.begin(), b_lower.end(), b_lower.begin(), ::tolower);

            // Exact match gets highest priority
            bool a_exact = (a_lower == query_lower);
            bool b_exact = (b_lower == query_lower);
            if (a_exact != b_exact) return a_exact > b_exact;

            // Starts with query gets next priority
            bool a_starts = (a_lower.find(query_lower) == 0);
            bool b_starts = (b_lower.find(query_lower) == 0);
            if (a_starts != b_starts) return a_starts > b_starts;

            // Contains in name (shorter names preferred)
            bool a_contains = (a_lower.find(query_lower) != std::string::npos);
            bool b_contains = (b_lower.find(query_lower) != std::string::npos);
            if (a_contains != b_contains) return a_contains > b_contains;

            // If both contain, prefer shorter names
            if (a_contains && b_contains) {
                return a.name.length() < b.name.length();
            }

            // Fallback to alphabetical
            return a_lower < b_lower;
        });
}

} // namespace paclook
