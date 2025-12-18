import "./SearchResults.css";

// ============================================
// SEARCH RESULTS COMPONENT
// Displays a list of search results as cards
// Handles loading state and empty results
// ============================================

// TypeScript interface for a single search result
interface SearchResult {
  docId: string;
  title: string;
  authors: string;
  abstract: string;
  score: number;
}

// Props interface
interface SearchResultsProps {
  results: SearchResult[]; // Array of search results
  loading: boolean; // Is search in progress?
  query: string; // Current search query
  hasSearched: boolean; // Has user searched yet?
}

function SearchResults({
  results,
  loading,
  query,
  hasSearched,
}: SearchResultsProps) {
  // Show loading spinner while searching
  if (loading) {
    return (
      <div className="results-container">
        <div className="loading">
          <div className="spinner"></div>
          <p>Searching...</p>
        </div>
      </div>
    );
  }

  // Show welcome message before first search
  if (!hasSearched) {
    return (
      <div className="results-container">
        <div className="welcome">
          <p>Enter a search term to find COVID-19 research papers</p>
        </div>
      </div>
    );
  }

  // Show "no results" message if search returned empty
  if (results.length === 0) {
    return (
      <div className="results-container">
        <div className="no-results">
          <p>
            No results found for "<strong>{query}</strong>"
          </p>
          <p>Try different keywords</p>
        </div>
      </div>
    );
  }

  // Render the list of results
  return (
    <div className="results-container">
      {/* Results header showing count */}
      <div className="results-header">
        <p>
          Found <strong>{results.length}</strong> results for "
          <strong>{query}</strong>"
        </p>
      </div>

      {/* Results list */}
      <div className="results-list">
        {results.map((result, index) => (
          <ResultCard
            key={result.docId || index}
            result={result}
            rank={index + 1}
          />
        ))}
      </div>
    </div>
  );
}

// ============================================
// RESULT CARD COMPONENT
// Displays a single search result
// ============================================

interface ResultCardProps {
  result: SearchResult;
  rank: number;
}

function ResultCard({ result, rank }: ResultCardProps) {
  // Truncate abstract to 300 characters for cleaner display
  const truncatedAbstract =
    result.abstract && result.abstract.length > 300
      ? result.abstract.substring(0, 300) + "..."
      : result.abstract;

  return (
    <article className="result-card">
      {/* Rank badge */}
      <span className="rank">#{rank}</span>

      {/* Paper title */}
      <h2 className="result-title">{result.title || "Untitled Document"}</h2>

      {/* Authors */}
      {result.authors && (
        <p className="result-authors">
          <strong>Authors:</strong> {result.authors}
        </p>
      )}

      {/* Abstract/Summary */}
      {truncatedAbstract && (
        <p className="result-abstract">{truncatedAbstract}</p>
      )}

      {/* Footer with metadata */}
      <div className="result-footer">
        <span className="doc-id">ID: {result.docId}</span>
        <span className="score">Score: {result.score.toFixed(2)}</span>
      </div>
    </article>
  );
}

export default SearchResults;
