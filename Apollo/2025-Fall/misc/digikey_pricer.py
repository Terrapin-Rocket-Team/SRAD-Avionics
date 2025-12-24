import csv
import requests
import time
import json

# NOTE: You'll need to get API credentials from Digikey
# Sign up at: https://developer.digikey.com/
# This script uses the Product Search API

def get_digikey_token(client_id, client_secret):
    """
    Get OAuth token from Digikey.
    You need to register at https://developer.digikey.com/
    """
    url = "https://api.digikey.com/v1/oauth2/token"
    headers = {"Content-Type": "application/x-www-form-urlencoded"}
    data = {
        "client_id": client_id,
        "client_secret": client_secret,
        "grant_type": "client_credentials"
    }
    
    response = requests.post(url, headers=headers, data=data)
    if response.status_code == 200:
        return response.json()["access_token"]
    else:
        print(f"Error getting token: {response.status_code}")
        return None

def search_digikey(part_number, token, client_id):
    """
    Search Digikey for a part number and return pricing info.
    """
    url = "https://api.digikey.com/products/v4/search/keyword"
    headers = {
        "Authorization": f"Bearer {token}",
        "X-DIGIKEY-Client-Id": client_id,
        "Content-Type": "application/json"
    }
    
    payload = {
        "Keywords": part_number,
        "Limit": 5,
        "Offset": 0
    }
    
    try:
        response = requests.post(url, headers=headers, json=payload)
        
        if response.status_code == 200:
            data = response.json()
            
            if data.get("ProductsCount", 0) > 0:
                products = data.get("Products", [])
                
                # Try to find exact or close match
                best_match = None
                part_number_lower = part_number.lower().replace('-', '').replace(' ', '')
                
                for product in products:
                    mfg_pn = product.get("ManufacturerProductNumber", "")
                    mfg_pn_normalized = mfg_pn.lower().replace('-', '').replace(' ', '')
                    
                    # Check for exact match (ignoring hyphens/spaces)
                    if mfg_pn_normalized == part_number_lower:
                        best_match = product
                        break
                    
                    # Check if it starts with the search term (for partial matches)
                    if mfg_pn_normalized.startswith(part_number_lower):
                        best_match = product
                        break
                
                # If no good match, check if search term is too generic
                if not best_match:
                    # Don't match on generic terms
                    generic_terms = ['buzzer', 'led', 'connector', 'switch', 'i2c', 'spi', 'uart']
                    if part_number.lower() in generic_terms:
                        return None
                    
                    # Otherwise take first result but warn
                    best_match = products[0]
                    print(f"  Warning: No exact match, using closest result")
                
                if best_match:
                    result = {
                        "part_number": best_match.get("ManufacturerProductNumber"),
                        "digikey_part": best_match.get("DigiKeyPartNumber"),
                        "manufacturer": best_match.get("Manufacturer", {}).get("Name"),
                        "description": best_match.get("Description", {}).get("ProductDescription"),
                        "stock": best_match.get("QuantityAvailable", 0),
                        "url": best_match.get("ProductUrl"),
                        "pricing": []
                    }
                    
                    # Extract pricing - could be UnitPrice or StandardPricing array
                    if "UnitPrice" in best_match:
                        # Single unit price
                        result["pricing"].append({
                            "quantity": 1,
                            "unit_price": best_match.get("UnitPrice")
                        })
                    
                    # Also check for StandardPricing array for bulk pricing
                    pricing = best_match.get("StandardPricing", [])
                    if pricing:
                        for tier in pricing:
                            result["pricing"].append({
                                "quantity": tier.get("BreakQuantity"),
                                "unit_price": tier.get("UnitPrice")
                            })
                    
                    return result
        
        else:
            print(f"  API Error: Status {response.status_code}")
            print(f"  Response: {response.text[:200]}")
        
        return None
        
    except Exception as e:
        print(f"Error searching for {part_number}: {e}")
        return None

def process_bom(input_file, output_file, token, client_id):
    """
    Process BOM CSV and add Digikey pricing information.
    """
    results = []
    
    with open(input_file, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f, delimiter=';')
        rows = list(reader)
    
    print(f"Processing {len(rows)} components...")
    
    for i, row in enumerate(rows, 1):
        footprint = row.get('Footprint', '').strip()
        designation = row.get('Designation', '').strip()
        quantity = row.get('Quantity', '').strip()
        
        print(f"\n[{i}/{len(rows)}] Searching: {designation} ({footprint})")
        
        # Skip passives for now (optional)
        if any(x in footprint.lower() for x in ['_0402_', '_0603_', '_0805_', '_1206_']):
            print("  Skipping passive component")
            results.append({
                'Footprint': footprint,
                'Quantity': quantity,
                'Designation': designation,
                'Digikey_PN': 'N/A - Passive',
                'Stock': '',
                'Unit_Price': '',
                'Ext_Price': '',
                'URL': ''
            })
            continue
        
        # Search Digikey
        part_info = search_digikey(designation, token, client_id)
        
        if part_info:
            # Get price for the quantity needed
            qty_needed = int(quantity) if quantity else 1
            unit_price = 0
            
            for tier in part_info['pricing']:
                if tier['quantity'] <= qty_needed:
                    unit_price = tier['unit_price']
            
            ext_price = unit_price * qty_needed if unit_price else 0
            
            results.append({
                'Footprint': footprint,
                'Quantity': quantity,
                'Designation': designation,
                'Manufacturer': part_info.get('manufacturer', ''),
                'Digikey_PN': part_info.get('digikey_part', ''),
                'Stock': part_info.get('stock', ''),
                'Unit_Price': f"${unit_price:.4f}",
                'Ext_Price': f"${ext_price:.2f}",
                'URL': part_info.get('url', '')
            })
            
            print(f"  Found: {part_info.get('part_number')} - ${unit_price:.4f} ea")
        else:
            print("  Not found")
            results.append({
                'Footprint': footprint,
                'Quantity': quantity,
                'Designation': designation,
                'Digikey_PN': 'NOT FOUND',
                'Stock': '',
                'Unit_Price': '',
                'Ext_Price': '',
                'URL': ''
            })
        
        # Rate limiting - be nice to the API
        time.sleep(0.5)
    
    # Write results to CSV
    with open(output_file, 'w', encoding='utf-8', newline='') as f:
        fieldnames = ['Footprint', 'Quantity', 'Designation', 'Manufacturer', 
                      'Digikey_PN', 'Stock', 'Unit_Price', 'Ext_Price', 'URL']
        writer = csv.DictWriter(f, fieldnames=fieldnames, delimiter=';')
        writer.writeheader()
        writer.writerows(results)
    
    # Calculate total
    total = sum(float(r['Ext_Price'].replace('$', '')) for r in results 
                if r['Ext_Price'] and r['Ext_Price'] != '')
    
    print(f"\n{'='*80}")
    print(f"Total estimated cost: ${total:.2f}")
    print(f"Results saved to: {output_file}")


if __name__ == "__main__":
    # Read credentials from digikey_credentials.txt
    # File format (one per line):
    # CLIENT_ID
    # CLIENT_SECRET
    
    credentials_file = "digikey_credentials.txt"
    
    try:
        with open(credentials_file, 'r') as f:
            lines = f.read().strip().split('\n')
            CLIENT_ID = lines[0].strip()
            CLIENT_SECRET = lines[1].strip()
    except FileNotFoundError:
        print(f"Error: {credentials_file} not found!")
        print("\nPlease create a file named 'digikey_credentials.txt' with:")
        print("Line 1: Your Client ID")
        print("Line 2: Your Client Secret")
        print("\nTo get credentials:")
        print("1. Sign up at https://developer.digikey.com/")
        print("2. Create an application to get Client ID and Secret")
        exit(1)
    except IndexError:
        print(f"Error: {credentials_file} is not formatted correctly!")
        print("File should have two lines:")
        print("Line 1: Your Client ID")
        print("Line 2: Your Client Secret")
        exit(1)
    
    input_file = "merged_output.csv"
    output_file = "bom_with_pricing.csv"
    
    print("Getting Digikey API token...")
    token = get_digikey_token(CLIENT_ID, CLIENT_SECRET)
    
    if token:
        print("Token obtained successfully!")
        process_bom(input_file, output_file, token, CLIENT_ID)
    else:
        print("Failed to get API token. Please check your credentials.")