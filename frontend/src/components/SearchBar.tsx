import { useState, useEffect, useRef } from "react";
import "./SearchBar.css";

// ============================================
// SEARCH BAR COMPONENT WITH AUTOCOMPLETE
// Features:
// - Real-time autocomplete suggestions from Trie
// - Keyboard navigation (up/down arrows, enter, escape)
// - Click-outside to close suggestions
// - Debounced API calls for performance
// ============================================

interface SearchBarProps {
  onSearch: (query: string) => void;
  initialQuery?: string;
}

// API base URL - centralized for easy configuration
const API_BASE = import.meta.env.VITE_API_URL;

function SearchBar({ onSearch, initialQuery = "" }: SearchBarProps) {
  // Input field value
  const [inputValue, setInputValue] = useState(initialQuery);
  // Autocomplete suggestions from API
  const [suggestions, setSuggestions] = useState<string[]>([]);
  // Currently highlighted suggestion index (-1 = none)
  const [highlightedIndex, setHighlightedIndex] = useState(-1);
  // Whether suggestions dropdown is visible
  const [showSuggestions, setShowSuggestions] = useState(false);
  // Loading state for suggestions
  const [isLoadingSuggestions, setIsLoadingSuggestions] = useState(false);

  // Refs for click-outside detection and focus management
  const containerRef = useRef<HTMLDivElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);

  // Sync with parent's initial query
  useEffect(() => {
    setInputValue(initialQuery);
  }, [initialQuery]);

  // Fetch autocomplete suggestions with debounce
  useEffect(() => {
    // Get the last word being typed for autocomplete
    const words = inputValue.trim().split(/\s+/);
    const lastWord = words[words.length - 1] || "";

    if (lastWord.length < 2) {
      setSuggestions([]);
      setShowSuggestions(false);
      return;
    }

    const debounceTimer = setTimeout(async () => {
      setIsLoadingSuggestions(true);
      try {
        const response = await fetch(
          `${API_BASE}/autocomplete?q=${encodeURIComponent(lastWord)}`
        );
        const data = await response.json();
        if (data.suggestions && data.suggestions.length > 0) {
          setSuggestions(data.suggestions);
          setShowSuggestions(true);
        } else {
          setSuggestions([]);
          setShowSuggestions(false);
        }
      } catch (error) {
        console.error("Autocomplete failed:", error);
        setSuggestions([]);
      } finally {
        setIsLoadingSuggestions(false);
      }
    }, 150); // 150ms debounce

    return () => clearTimeout(debounceTimer);
  }, [inputValue]);

  // Close suggestions when clicking outside
  useEffect(() => {
    const handleClickOutside = (event: MouseEvent) => {
      if (
        containerRef.current &&
        !containerRef.current.contains(event.target as Node)
      ) {
        setShowSuggestions(false);
      }
    };

    document.addEventListener("mousedown", handleClickOutside);
    return () => document.removeEventListener("mousedown", handleClickOutside);
  }, []);

  // Handle form submission
  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (inputValue.trim()) {
      setShowSuggestions(false);
      onSearch(inputValue.trim());
    }
  };

  // Handle keyboard navigation in suggestions
  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (!showSuggestions || suggestions.length === 0) {
      if (e.key === "Enter") {
        handleSubmit(e);
      }
      return;
    }

    switch (e.key) {
      case "ArrowDown":
        e.preventDefault();
        setHighlightedIndex((prev) =>
          prev < suggestions.length - 1 ? prev + 1 : 0
        );
        break;
      case "ArrowUp":
        e.preventDefault();
        setHighlightedIndex((prev) =>
          prev > 0 ? prev - 1 : suggestions.length - 1
        );
        break;
      case "Enter":
        e.preventDefault();
        if (highlightedIndex >= 0) {
          selectSuggestion(suggestions[highlightedIndex]);
        } else {
          handleSubmit(e);
        }
        break;
      case "Escape":
        setShowSuggestions(false);
        setHighlightedIndex(-1);
        break;
      case "Tab":
        if (highlightedIndex >= 0) {
          e.preventDefault();
          selectSuggestion(suggestions[highlightedIndex]);
        }
        break;
    }
  };

  // Select a suggestion and replace the last word
  const selectSuggestion = (suggestion: string) => {
    const words = inputValue.trim().split(/\s+/);
    words[words.length - 1] = suggestion;
    const newValue = words.join(" ") + " ";
    setInputValue(newValue);
    setShowSuggestions(false);
    setHighlightedIndex(-1);
    inputRef.current?.focus();
  };

  // Handle input change
  const handleInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    setInputValue(e.target.value);
    setHighlightedIndex(-1);
  };

  return (
    <div className="search-bar-container" ref={containerRef}>
      <form className="search-bar" onSubmit={handleSubmit}>
        {/* Search Icon */}
        <div className="search-icon">
          <svg
            width="20"
            height="20"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            strokeWidth="2"
            strokeLinecap="round"
            strokeLinejoin="round"
          >
            <circle cx="11" cy="11" r="8" />
            <path d="m21 21-4.35-4.35" />
          </svg>
        </div>

        {/* Input Field */}
        <input
          ref={inputRef}
          type="text"
          className="search-input"
          placeholder="Search COVID-19 research papers..."
          value={inputValue}
          onChange={handleInputChange}
          onKeyDown={handleKeyDown}
          onFocus={() => suggestions.length > 0 && setShowSuggestions(true)}
          autoComplete="off"
          spellCheck="false"
        />

        {/* Clear Button */}
        {inputValue && (
          <button
            type="button"
            className="clear-button"
            onClick={() => {
              setInputValue("");
              setSuggestions([]);
              inputRef.current?.focus();
            }}
            aria-label="Clear search"
          >
            <svg
              width="18"
              height="18"
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              strokeWidth="2"
              strokeLinecap="round"
              strokeLinejoin="round"
            >
              <path d="M18 6 6 18" />
              <path d="m6 6 12 12" />
            </svg>
          </button>
        )}

        {/* Search Button */}
        <button
          type="submit"
          className="search-button"
          disabled={!inputValue.trim()}
        >
          Search
        </button>
      </form>

      {/* Autocomplete Suggestions Dropdown */}
      {showSuggestions && suggestions.length > 0 && (
        <div className="suggestions-dropdown">
          {isLoadingSuggestions && (
            <div className="suggestions-loading">Loading...</div>
          )}
          <ul className="suggestions-list">
            {suggestions.map((suggestion, index) => (
              <li
                key={suggestion}
                className={`suggestion-item ${
                  index === highlightedIndex ? "highlighted" : ""
                }`}
                onClick={() => selectSuggestion(suggestion)}
                onMouseEnter={() => setHighlightedIndex(index)}
              >
                <svg
                  className="suggestion-icon"
                  width="16"
                  height="16"
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  strokeWidth="2"
                >
                  <circle cx="11" cy="11" r="8" />
                  <path d="m21 21-4.35-4.35" />
                </svg>
                <span className="suggestion-text">{suggestion}</span>
              </li>
            ))}
          </ul>
          <div className="suggestions-hint">
            <kbd>↑</kbd> <kbd>↓</kbd> to navigate, <kbd>Enter</kbd> to select,{" "}
            <kbd>Esc</kbd> to close
          </div>
        </div>
      )}
    </div>
  );
}

export default SearchBar;
