import os

# Change working directory to script's location
script_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(script_dir)

def concatenate_csv_files(root_path, output_file):
    """
    Recursively find all CSV files in root_path and its subfolders,
    then append all lines (except headers) to a single output file.
    
    Args:
        root_path: Path to start searching for CSV files
        output_file: Path to the output file
    """
    csv_count = 0
    total_lines = 0
    
    # Open output file in write mode
    with open(output_file, 'w', encoding='utf-8') as outfile:
        # Walk through all directories and subdirectories
        for dirpath, dirnames, filenames in os.walk(root_path):
            # Filter for CSV files
            csv_files = [f for f in filenames if f.lower().endswith('.csv')]
            
            for csv_file in csv_files:
                csv_path = os.path.join(dirpath, csv_file)\
                
                try:
                    with open(csv_path, 'r', encoding='utf-8') as infile:
                        lines = infile.readlines()
                        
                        # Skip the first line (header) and write the rest
                        if len(lines) > 1:
                            outfile.writelines(lines[(1 if csv_count > 0 else 0):])
                            total_lines += len(lines) - 1
                            print(f"Processed: {csv_path} ({len(lines) - 1} lines)")
                            csv_count += 1
                        else:
                            print(f"Skipped (no data): {csv_path}")
                            
                except Exception as e:
                    print(f"Error processing {csv_path}: {e}")
    
    print(f"\nSummary:")
    print(f"CSV files processed: {csv_count}")
    print(f"Total lines written: {total_lines}")
    print(f"Output saved to: {output_file}")


if __name__ == "__main__":
    # Set your paths here
    root_path = "../"  # Change this to your folder path
    output_file = "combined_output.csv"  # Change this to your desired output filename
    
    # Check if root path exists
    if not os.path.exists(root_path):
        print(f"Error: Path '{root_path}' does not exist!")
    else:
        concatenate_csv_files(root_path, output_file)