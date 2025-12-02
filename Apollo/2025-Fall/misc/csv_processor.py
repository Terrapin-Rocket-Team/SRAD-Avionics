import csv
import re

def normalize_value(value):
    """
    Normalize component values for comparison.
    E.g., '100nF', '100 nF', '100nf', '100n' all become '100nf'
    '5.1k', '5k1', '5.1 k', '5.1K' all become '5.1k'
    '2.2uF', '2u2' all become '2.2uf'
    """
    if not value:
        return ''
    
    # Convert to lowercase and remove spaces
    normalized = value.lower().replace(' ', '')
    
    # Handle resistance values with omega symbol
    normalized = normalized.replace('ω', '')
    
    # Remove trailing 'r' (used in some notation like '5.1kR')
    if normalized.endswith('r'):
        normalized = normalized[:-1]
    
    # Handle u2 notation -> .2u (e.g., 2u2 -> 2.2u)
    if 'u' in normalized and not 'uf' in normalized:
        parts = normalized.split('u')
        if len(parts) == 2 and parts[1] and parts[1][0].isdigit():
            # Convert 2u2 to 2.2uf
            normalized = parts[0] + '.' + parts[1][0] + 'uf' + parts[1][1:]
        elif len(parts) == 2 and not parts[1]:
            # Convert 15u to 15uf
            normalized = parts[0] + 'uf'
    
    # Handle k1 notation -> .1k (e.g., 5k1 -> 5.1k)
    if 'k' in normalized:
        parts = normalized.split('k')
        if len(parts) == 2 and parts[1] and parts[1][0].isdigit():
            # Convert 5k1 to 5.1k
            normalized = parts[0] + '.' + parts[1][0] + 'k' + parts[1][1:]
    
    # Handle 100n -> 100nf
    if normalized.endswith('n') and not normalized.endswith('nf'):
        normalized = normalized + 'f'
    
    return normalized

def is_passive(footprint):
    """Check if component is a passive (resistor, capacitor, inductor)"""
    footprint_lower = footprint.lower()
    return any(x in footprint_lower for x in ['_0402_', '_0603_', '_0805_', '_1206_', '_1210_', 'c_0', 'r_0', 'l_1'])

def is_mergeable_by_footprint(footprint):
    """Check if component should be merged by footprint only (LEDs, buzzers)"""
    footprint_lower = footprint.lower()
    return 'led_' in footprint_lower or 'buzzer' in footprint_lower

def is_ic_or_active(footprint, designation):
    """Check if component is an IC or active component"""
    # If it's a passive, it's not an IC
    if is_passive(footprint):
        return False
    
    # Connectors, LEDs, mechanical parts are not ICs
    footprint_lower = footprint.lower()
    designation_lower = designation.lower()
    
    non_ic_keywords = ['jst', 'connector', 'led', 'buzzer', 'teensy', 'switch', 'button', 
                       'conn_', 'battery', 'coaxial']
    
    for keyword in non_ic_keywords:
        if keyword in footprint_lower or keyword in designation_lower:
            return False
    
    return True

def get_merge_key(footprint, designation):
    """
    Generate a key for merging:
    - For LEDs and Buzzers: (footprint) only - all same footprint merge
    - For passives (R/C/L): (footprint, normalized_value) - merge by size AND value
    - For ICs: (normalized_designation)
    - For others: (footprint, designation)
    """
    if is_mergeable_by_footprint(footprint):
        # Merge LEDs and buzzers by footprint only
        return ('by_footprint', footprint)
    elif is_passive(footprint):
        # Merge passives by footprint and normalized value
        return ('passive', footprint, normalize_value(designation))
    elif is_ic_or_active(footprint, designation):
        # Merge ICs by normalized designation only
        return ('ic', normalize_value(designation))
    else:
        # Other components: merge by exact footprint and designation
        return ('other', footprint, designation)

def process_csv(input_file, output_file):
    """
    Process CSV by:
    1. Removing first column (Id)
    2. Removing Designator column
    3. Intelligently merging rows:
       - Passives: by footprint + normalized value
       - ICs: by normalized designation
       - Others: by exact match
    4. Summing quantities for merged rows
    5. Dropping empty columns
    """
    
    # Read the CSV file
    with open(input_file, 'r', encoding='utf-8') as f:
        reader = csv.reader(f, delimiter=';')
        rows = list(reader)
    
    if not rows:
        print("Empty file!")
        return
    
    # Get header (skip first column "Id")
    header = [col.strip('"') for col in rows[0][1:]]
    
    # Find column indices
    designator_idx = header.index('Designator') if 'Designator' in header else None
    footprint_idx = header.index('Footprint') if 'Footprint' in header else None
    quantity_idx = header.index('Quantity') if 'Quantity' in header else None
    designation_idx = header.index('Designation') if 'Designation' in header else None
    
    # Dictionary to store merged data
    merged_data = {}
    
    # Track total quantities
    total_qty_before = 0
    
    # Process each data row
    for row in rows[1:]:
        if len(row) <= 1:
            continue
            
        # Remove first column (Id)
        row_data = [col.strip('"') for col in row[1:]]
        
        if len(row_data) < len(header):
            continue
        
        # Get key fields
        footprint = row_data[footprint_idx] if footprint_idx is not None else ''
        designation = row_data[designation_idx] if designation_idx is not None else ''
        quantity = int(row_data[quantity_idx]) if quantity_idx is not None and row_data[quantity_idx] else 0
        
        # Skip JST connectors
        if 'jst' in footprint.lower():
            continue
        
        # Add to total before merging
        total_qty_before += quantity
        
        # Create merge key based on component type
        merge_key = get_merge_key(footprint, designation)
        
        # Initialize if new key
        if merge_key not in merged_data:
            merged_data[merge_key] = {
                'Quantity': 0, 
                'Footprint': footprint,
                'Designation': designation,
                'data': {}
            }
        
        # Sum quantities
        merged_data[merge_key]['Quantity'] += quantity
        
        # Store other column data (first occurrence wins for non-quantity fields)
        if not merged_data[merge_key]['data']:
            for i, col_name in enumerate(header):
                if col_name not in ['Designator', 'Quantity', 'Footprint', 'Designation']:
                    merged_data[merge_key]['data'][col_name] = row_data[i]
    
    # Create output header (remove Designator, keep rest)
    output_header = [col for col in header if col != 'Designator']
    
    # Check which columns have any data
    columns_with_data = set(output_header)
    for data in merged_data.values():
        for col in list(columns_with_data):
            if col in ['Footprint', 'Quantity', 'Designation']:
                continue
            if col in data['data'] and data['data'][col]:
                break
        else:
            if col not in ['Footprint', 'Quantity', 'Designation']:
                columns_with_data.discard(col)
    
    # Filter header to only columns with data
    output_header = [col for col in output_header if col in columns_with_data]
    
    # Calculate total quantity after merging
    total_qty_after = sum(info['Quantity'] for info in merged_data.values())
    
    # Write output CSV
    with open(output_file, 'w', encoding='utf-8', newline='') as f:
        writer = csv.writer(f, delimiter=';')
        
        # Write header
        writer.writerow(output_header)
        
        # Write merged data, sorted by footprint then designation
        for merge_key, info in sorted(merged_data.items(), key=lambda x: (x[1]['Footprint'], x[1]['Designation'])):
            row_output = []
            for col in output_header:
                if col == 'Quantity':
                    row_output.append(str(info['Quantity']))
                elif col == 'Footprint':
                    row_output.append(info['Footprint'])
                elif col == 'Designation':
                    row_output.append(info['Designation'])
                else:
                    row_output.append(info['data'].get(col, ''))
            writer.writerow(row_output)
    
    print(f"Processed {len(rows)-1} rows into {len(merged_data)} unique components")
    print(f"Total quantity before merge: {total_qty_before}")
    print(f"Total quantity after merge: {total_qty_after}")
    
    if total_qty_before == total_qty_after:
        print("✓ Quantity verification PASSED - totals match!")
    else:
        print(f"✗ WARNING: Quantity mismatch! Difference: {total_qty_after - total_qty_before}")
    
    print(f"Output saved to: {output_file}")


if __name__ == "__main__":
    input_file = "combined_output.csv"  # Change to your input file
    output_file = "merged_output.csv"  # Change to your desired output file
    
    process_csv(input_file, output_file)