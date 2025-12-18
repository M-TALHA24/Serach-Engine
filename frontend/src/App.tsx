import { useState } from "react";
import SearchBar from "./components/SearchBar";
import SearchResults from "./components/SearchResults";
import "./App.css";

// ============================================
// MAIN APP COMPONENT
// This is the root component of our search engine UI
// It manages the global state and coordinates child components
// ============================================

// TypeScript interface to define the shape of a search result
interface SearchResult {
  docId: string; // Unique document identifier
  title: string; // Paper title
  authors: string; // Paper authors
  abstract: string; // Paper abstract/summary
  score: number; // Relevance score from our ranking algorithm
}

function App() {
  // ========== STATE MANAGEMENT ==========
  // Using React's useState hook to manage application state

  // Stores the current search query
  const [query, setQuery] = useState("");

  // Stores the array of search results
  const [results, setResults] = useState<SearchResult[]>([]);

  // Tracks if a search request is in progress (for loading spinner)
  const [loading, setLoading] = useState(false);

  // Tracks if user has performed at least one search
  const [hasSearched, setHasSearched] = useState(false);

  // ========== SEARCH HANDLER ==========
  // This function is called when user submits a search query
  const handleSearch = async (searchQuery: string) => {
    // Validate: don't search if query is empty or just whitespace
    if (!searchQuery.trim()) return;

    // Update state to reflect new search
    setQuery(searchQuery);
    setLoading(true); // Show loading indicator
    setHasSearched(true); // Mark that we've searched

    try {
      // Make HTTP GET request to our C++ backend server
      // encodeURIComponent ensures special characters are properly escaped
      const response = await fetch(
        `http://localhost:5000/search?q=${encodeURIComponent(searchQuery)}`
      );

      // Parse JSON response from server
      const data = await response.json();

      // Update results state (use empty array if no results)
      setResults(data.results || []);
    } catch (error) {
      // Handle network errors or server issues
      console.error("Search failed:", error);
      setResults([]);
    } finally {
      // Always hide loading indicator when done
      setLoading(false);
    }
  };

  // ========== RENDER UI ==========
  return (
    <div className="app">
      {/* Header Section - Contains logo and tagline */}
      <header className="header">
        <h1
          className="logo"
          onClick={() => {
            setHasSearched(false);
            setResults([]);
          }}
        >
          CORD-19 Search
        </h1>
        <p className="tagline">Search COVID-19 Research Papers</p>
      </header>

      {/* Search Bar - Handles user input */}
      <SearchBar onSearch={handleSearch} />

      {/* Search Results - Displays matching documents */}
      <SearchResults
        results={results}
        loading={loading}
        query={query}
        hasSearched={hasSearched}
      />
    </div>
  );
}

export default App;
