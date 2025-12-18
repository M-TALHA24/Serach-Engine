import { useState, useCallback } from "react";
import SearchBar from "./components/SearchBar";
import SearchResults from "./components/SearchResults";
import "./App.css";

// ============================================
// CORD-19 SEARCH ENGINE
// A professional search interface for COVID-19 research papers
// Features:
// - Autocomplete suggestions
// - Advanced search options (AND/OR modes)
// - Responsive design
// - Error handling with retry
// ============================================

// API Configuration
const API_BASE = "http://localhost:5000";

// TypeScript interfaces
interface SearchResult {
  docId: string;
  title: string;
  authors: string;
  abstract: string;
  score: number;
}

type SearchMode = "or" | "and";

function App() {
  // ========== STATE MANAGEMENT ==========
  const [query, setQuery] = useState("");
  const [results, setResults] = useState<SearchResult[]>([]);
  const [loading, setLoading] = useState(false);
  const [hasSearched, setHasSearched] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [searchMode, setSearchMode] = useState<SearchMode>("or");
  const [searchTime, setSearchTime] = useState<number | null>(null);
  const [totalResults, setTotalResults] = useState(0);

  // ========== SEARCH HANDLER ==========
  const handleSearch = useCallback(
    async (searchQuery: string) => {
      if (!searchQuery.trim()) return;

      setQuery(searchQuery);
      setLoading(true);
      setHasSearched(true);
      setError(null);

      const startTime = performance.now();

      try {
        // Add search mode to query if AND mode
        const finalQuery =
          searchMode === "and"
            ? searchQuery
                .split(/\s+/)
                .filter((w) => w.length > 0)
                .join(" AND ")
            : searchQuery;

        const response = await fetch(
          `${API_BASE}/search?q=${encodeURIComponent(finalQuery)}`
        );

        if (!response.ok) {
          throw new Error(`Server error: ${response.status}`);
        }

        const data = await response.json();
        const endTime = performance.now();

        setResults(data.results || []);
        setTotalResults(data.results?.length || 0);
        setSearchTime(endTime - startTime);
      } catch (err) {
        console.error("Search failed:", err);
        setError(
          err instanceof Error
            ? err.message
            : "Failed to connect to search server"
        );
        setResults([]);
      } finally {
        setLoading(false);
      }
    },
    [searchMode]
  );

  // ========== RESET TO HOME ==========
  const handleReset = () => {
    setHasSearched(false);
    setResults([]);
    setQuery("");
    setError(null);
    setSearchTime(null);
  };

  // ========== RETRY SEARCH ==========
  const handleRetry = () => {
    if (query) {
      handleSearch(query);
    }
  };

  // ========== RENDER UI ==========
  return (
    <div className="app">
      {/* Header Section */}
      <header className={`header ${hasSearched ? "header-compact" : ""}`}>
        <div className="header-content">
          {/* Logo */}
          <h1 className="logo" onClick={handleReset}>
            <span className="logo-icon">🔬</span>
            <span className="logo-text">CORD-19</span>
            <span className="logo-search">Search</span>
          </h1>

          {/* Tagline - only show on home */}
          {!hasSearched && (
            <p className="tagline">
              Explore the COVID-19 Open Research Dataset
              <br />
              <span className="tagline-sub">
                Search through thousands of scholarly articles about COVID-19
                and related coronaviruses
              </span>
            </p>
          )}
        </div>
      </header>

      {/* Main Content Area */}
      <main className="main-content">
        {/* Search Controls */}
        <div className={`search-section ${hasSearched ? "search-sticky" : ""}`}>
          <SearchBar onSearch={handleSearch} initialQuery={query} />

          {/* Search Mode Toggle */}
          <div className="search-options">
            <div className="mode-toggle">
              <span className="mode-label">Search mode:</span>
              <button
                className={`mode-btn ${searchMode === "or" ? "active" : ""}`}
                onClick={() => setSearchMode("or")}
              >
                Any word (OR)
              </button>
              <button
                className={`mode-btn ${searchMode === "and" ? "active" : ""}`}
                onClick={() => setSearchMode("and")}
              >
                All words (AND)
              </button>
            </div>
          </div>
        </div>

        {/* Error State */}
        {error && (
          <div className="error-container">
            <div className="error-card">
              <div className="error-icon">⚠️</div>
              <h3>Connection Error</h3>
              <p>{error}</p>
              <p className="error-hint">
                Make sure the API server is running on port 5000
              </p>
              <button className="retry-button" onClick={handleRetry}>
                Try Again
              </button>
            </div>
          </div>
        )}

        {/* Results Section */}
        {!error && (
          <SearchResults
            results={results}
            loading={loading}
            query={query}
            hasSearched={hasSearched}
            searchTime={searchTime}
            totalResults={totalResults}
          />
        )}
      </main>

      {/* Footer */}
      <footer className="footer">
        <p>
          Built with C++ (Backend) and React (Frontend) •{" "}
          <a
            href="https://www.semanticscholar.org/cord19"
            target="_blank"
            rel="noopener noreferrer"
          >
            CORD-19 Dataset
          </a>
        </p>
      </footer>
    </div>
  );
}

export default App;
