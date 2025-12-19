import { useState } from "react";
import "./SearchResults.css";

// ============================================
// SEARCH RESULTS COMPONENT
// Professional display of search results with:
// - Pagination
// - Expandable abstracts
// - Search statistics
// - Smooth animations
// ============================================

interface SearchResult {
  docId: string;
  title: string;
  authors: string;
  abstract: string;
  url: string;
  score: number;
}

interface SearchResultsProps {
  results: SearchResult[];
  loading: boolean;
  query: string;
  hasSearched: boolean;
  searchTime: number | null;
  totalResults: number;
}

// Number of results per page
const RESULTS_PER_PAGE = 10;

function SearchResults({
  results,
  loading,
  query,
  hasSearched,
  searchTime,
  totalResults,
}: SearchResultsProps) {
  const [currentPage, setCurrentPage] = useState(1);

  // Reset to first page when results change
  if (
    results.length > 0 &&
    currentPage > Math.ceil(results.length / RESULTS_PER_PAGE)
  ) {
    setCurrentPage(1);
  }

  // Calculate pagination
  const totalPages = Math.ceil(results.length / RESULTS_PER_PAGE);
  const startIndex = (currentPage - 1) * RESULTS_PER_PAGE;
  const paginatedResults = results.slice(
    startIndex,
    startIndex + RESULTS_PER_PAGE
  );

  // Loading state
  if (loading) {
    return (
      <div className="results-container">
        <div className="loading-state">
          <div className="loading-spinner">
            <div className="spinner-ring"></div>
            <div className="spinner-ring"></div>
            <div className="spinner-ring"></div>
          </div>
          <p className="loading-text">Searching research papers...</p>
          <p className="loading-subtext">Querying the CORD-19 database</p>
        </div>
      </div>
    );
  }

  // Welcome state (before first search)
  if (!hasSearched) {
    return (
      <div className="results-container">
        <div className="welcome-state">
          <div className="welcome-icon">📚</div>
          <h2>Search COVID-19 Research</h2>
          <p>Enter keywords to find relevant scholarly articles</p>
          <div className="welcome-suggestions">
            <p className="suggestions-label">Try searching for:</p>
            <div className="suggestion-chips">
              <span className="chip">vaccine efficacy</span>
              <span className="chip">transmission dynamics</span>
              <span className="chip">treatment protocols</span>
              <span className="chip">immunity response</span>
            </div>
          </div>
        </div>
      </div>
    );
  }

  // No results state
  if (results.length === 0) {
    return (
      <div className="results-container">
        <div className="no-results-state">
          <div className="no-results-icon">🔍</div>
          <h3>No results found</h3>
          <p>
            We couldn't find any papers matching "<strong>{query}</strong>"
          </p>
          <div className="no-results-tips">
            <p>Suggestions:</p>
            <ul>
              <li>Check your spelling</li>
              <li>Try more general keywords</li>
              <li>Use fewer words in your search</li>
              <li>Try switching to "Any word (OR)" mode</li>
            </ul>
          </div>
        </div>
      </div>
    );
  }

  // Results state
  return (
    <div className="results-container">
      {/* Results Header with Stats */}
      <div className="results-header">
        <div className="results-stats">
          <span className="results-count">
            <strong>{totalResults}</strong> results
          </span>
          {searchTime && (
            <span className="search-time">
              ({(searchTime / 1000).toFixed(2)} seconds)
            </span>
          )}
        </div>
        <p className="results-query">
          Showing results for "<strong>{query}</strong>"
        </p>
      </div>

      {/* Results List */}
      <div className="results-list">
        {paginatedResults.map((result, index) => (
          <ResultCard
            key={result.docId || `result-${startIndex + index}`}
            result={result}
            rank={startIndex + index + 1}
          />
        ))}
      </div>

      {/* Pagination */}
      {totalPages > 1 && (
        <div className="pagination">
          <button
            className="page-btn prev"
            onClick={() => setCurrentPage((p) => Math.max(1, p - 1))}
            disabled={currentPage === 1}
          >
            ← Previous
          </button>

          <div className="page-numbers">
            {Array.from({ length: totalPages }, (_, i) => i + 1)
              .filter((page) => {
                // Show first, last, and pages around current
                return (
                  page === 1 ||
                  page === totalPages ||
                  Math.abs(page - currentPage) <= 1
                );
              })
              .map((page, idx, arr) => (
                <span key={page}>
                  {idx > 0 && arr[idx - 1] !== page - 1 && (
                    <span className="page-ellipsis">...</span>
                  )}
                  <button
                    className={`page-num ${
                      currentPage === page ? "active" : ""
                    }`}
                    onClick={() => setCurrentPage(page)}
                  >
                    {page}
                  </button>
                </span>
              ))}
          </div>

          <button
            className="page-btn next"
            onClick={() => setCurrentPage((p) => Math.min(totalPages, p + 1))}
            disabled={currentPage === totalPages}
          >
            Next →
          </button>
        </div>
      )}
    </div>
  );
}

// ============================================
// RESULT CARD COMPONENT
// Individual search result with expandable abstract
// ============================================

interface ResultCardProps {
  result: SearchResult;
  rank: number;
}

function ResultCard({ result, rank }: ResultCardProps) {
  const [isExpanded, setIsExpanded] = useState(false);

  // Determine if abstract needs truncation
  const abstractLength = result.abstract?.length || 0;
  const needsTruncation = abstractLength > 300;

  const displayAbstract =
    needsTruncation && !isExpanded
      ? result.abstract.substring(0, 300) + "..."
      : result.abstract;

  // Format score as percentage for user-friendly display
  const scoreDisplay = Math.min(result.score, 100).toFixed(1);

  return (
    <article className="result-card">
      {/* Card Header */}
      <div className="card-header">
        <span className="rank-badge">#{rank}</span>
        <div className="relevance-score">
          <div
            className="score-bar"
            style={{ width: `${Math.min(result.score * 5, 100)}%` }}
          />
          <span className="score-label">{scoreDisplay} relevance</span>
        </div>
      </div>

      {/* Title */}
      <h2 className="result-title">{result.title || "Untitled Document"}</h2>

      {/* Authors */}
      {result.authors && (
        <p className="result-authors">
          <span className="authors-icon">👤</span>
          {result.authors}
        </p>
      )}

      {/* Abstract */}
      {result.abstract && (
        <div className="result-abstract-container">
          <p className="result-abstract">{displayAbstract}</p>
          {needsTruncation && (
            <button
              className="expand-btn"
              onClick={() => setIsExpanded(!isExpanded)}
            >
              {isExpanded ? "Show less" : "Read more"}
            </button>
          )}
        </div>
      )}

      {/* Footer */}
      <div className="card-footer">
        {result.url ? (
          <a
            href={result.url}
            target="_blank"
            rel="noopener noreferrer"
            className="paper-link"
          >
            <span className="link-icon">📄</span>
            View Full Paper on PubMed
            <span className="external-icon">↗</span>
          </a>
        ) : (
          <span className="doc-id">
            <span className="id-label">Document ID:</span>
            <code>{result.docId}</code>
          </span>
        )}
      </div>
    </article>
  );
}

export default SearchResults;
