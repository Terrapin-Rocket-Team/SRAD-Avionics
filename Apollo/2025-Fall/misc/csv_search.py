import os
import csv

def search_csvs(root_path, search_term, case_sensitive=False):
    """
    Recursively search all CSV files in root_path for a given term.
    
    Args:
        root_path: Path to start searching for CSV files
        search_term: Term to search for
        case_sensitive: Whether search should be case-sensitive (default: False)
    """
    csv_count = 0
    total_matches = 0
    
    # Prepare search term
    if not case_sensitive:
        search_term = search_term.lower()
    
    print(f"Searching for: '{search_term}'")
    print(f"Case sensitive: {case_sensitive}")
    print("-" * 80)
    
    # Walk through all directories and subdirectories
    for dirpath, dirnames, filenames in os.walk(root_path):
        # Filter for CSV files
        csv_files = [f for f in filenames if f.lower().endswith('.csv')]
        
        for csv_file in csv_files:
            csv_path = os.path.join(dirpath, csv_file)
            csv_count += 1
            matches_in_file = []
            
            try:
                with open(csv_path, 'r', encoding='utf-8') as f:
                    reader = csv.reader(f, delimiter=';')
                    
                    for row_num, row in enumerate(reader, start=1):
                        # Search in each cell of the row
                        for col_num, cell in enumerate(row):
                            cell_content = cell.strip('"')
                            search_content = cell_content if case_sensitive else cell_content.lower()
                            
                            if search_term in search_content:
                                matches_in_file.append({
                                    'row': row_num,
                                    'col': col_num,
                                    'content': cell_content,
                                    'full_row': row
                                })
                
                # Print results for this file if matches found
                if matches_in_file:
                    print(f"\n📄 {csv_path}")
                    print(f"   Found {len(matches_in_file)} match(es):")
                    
                    for match in matches_in_file:
                        print(f"   Row {match['row']}, Col {match['col']}: {match['content']}")
                        # Optionally print the full row for context
                        # print(f"      Full row: {match['full_row']}")
                    
                    total_matches += len(matches_in_file)
                    
            except Exception as e:
                print(f"Error reading {csv_path}: {e}")
    
    print("\n" + "=" * 80)
    print(f"Summary:")
    print(f"CSV files searched: {csv_count}")
    print(f"Total matches found: {total_matches}")


if __name__ == "__main__":
    # Set your parameters here
    root_path = "../"  # Current directory, change as needed
    search_term = "XC6220B331MR"  # Change this to what you're searching for
    case_sensitive = False  # Set to True for case-sensitive search
    
    # Check if root path exists
    if not os.path.exists(root_path):
        print(f"Error: Path '{root_path}' does not exist!")
    else:
        search_csvs(root_path, search_term, case_sensitive)
