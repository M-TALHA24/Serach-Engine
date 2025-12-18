import { useState } from "react";
import "./SearchBar.css";

// ============================================
// SEARCH BAR COMPONENT
// A simple input field with a search button
// Uses controlled component pattern (React manages input value)
// ============================================

// Props interface - defines what props this component accepts
interface SearchBarProps {
  onSearch: (query: string) => void; // Callback function when user searches
}

function SearchBar({ onSearch }: SearchBarProps) {
  // Local state to store the current input value
  const [inputValue, setInputValue] = useState("");

  // Handle form submission
  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault(); // Prevent page reload on form submit
    onSearch(inputValue); // Call parent's search handler
  };

  return (
    <form className="search-bar" onSubmit={handleSubmit}>
      {/* Search Input Field */}
      <input
        type="text"
        className="search-input"
        placeholder="Search for research papers..."
        value={inputValue}
        onChange={(e) => setInputValue(e.target.value)}
      />

      {/* Search Button */}
      <button type="submit" className="search-button">
        Search
      </button>
    </form>
  );
}

export default SearchBar;
