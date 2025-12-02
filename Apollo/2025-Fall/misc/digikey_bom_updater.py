#!/usr/bin/env python3
"""
DigiKey BOM Updater
Fetches pricing and stock information from DigiKey API and updates your BOM
"""

import os
import sys
import csv
import requests
from urllib.parse import urlparse, parse_qs
from typing import Dict, List, Optional

# DigiKey API Configuration
DIGIKEY_API_BASE = "https://api.digikey.com"
TOKEN_URL = f"{DIGIKEY_API_BASE}/v1/oauth2/token"
PRODUCT_DETAILS_URL = f"{DIGIKEY_API_BASE}/products/v4/search"


class DigiKeyAPI:
    def __init__(self, client_id: str, client_secret: str):
        self.client_id = client_id
        self.client_secret = client_secret
        self.access_token = None
        
    def get_access_token(self) -> str:
        """Get OAuth2 access token"""
        data = {
            'client_id': self.client_id,
            'client_secret': self.client_secret,
            'grant_type': 'client_credentials'
        }
        
        response = requests.post(TOKEN_URL, data=data)
        response.raise_for_status()
        
        self.access_token = response.json()['access_token']
        return self.access_token
    
    def extract_part_number_from_url(self, url: str) -> Optional[str]:
        """Extract DigiKey part number from URL"""
        if not url or 'digikey.com' not in url:
            return None
            
        try:
            # Parse URL to get part number from path or query
            parsed = urlparse(url)
            path_parts = parsed.path.split('/')
            
            # Try to find part number in path
            if 'products' in path_parts and 'detail' in path_parts:
                detail_idx = path_parts.index('detail')
                if detail_idx + 2 < len(path_parts):
                    return path_parts[detail_idx + 2]
            
            # Try query parameters
            query_params = parse_qs(parsed.query)
            if 's' in query_params:
                return query_params['s'][0]
                
        except Exception as e:
            print(f"Error parsing URL {url}: {e}")
        
        return None
    
    def get_product_info(self, part_number: str) -> Optional[Dict]:
        """Get product information from DigiKey API"""
        if not self.access_token:
            self.get_access_token()
        
        headers = {
            'Authorization': f'Bearer {self.access_token}',
            'X-DIGIKEY-Client-Id': self.client_id,
            'Content-Type': 'application/json'
        }
        
        # Use keyword search endpoint
        payload = {
            'Keywords': part_number,
            'SearchOptions': ['ManufacturerPartSearch'],
            'RecordCount': 1
        }
        
        try:
            response = requests.post(
                f"{DIGIKEY_API_BASE}/products/v4/search/keyword",
                headers=headers,
                json=payload
            )
            
            if response.status_code == 200:
                data = response.json()
                # Get the first product from results
                if 'Products' in data and len(data['Products']) > 0:
                    return data['Products'][0]
                else:
                    print(f"No results found for {part_number}")
                    return None
            else:
                print(f"API Error for {part_number}: {response.status_code} - {response.text[:200]}")
                return None
                
        except Exception as e:
            print(f"Error fetching {part_number}: {e}")
            return None


def update_bom(input_file: str, output_file: str, api: DigiKeyAPI):
    """Update BOM with DigiKey pricing information"""
    
    with open(input_file, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)  # Default delimiter is comma
        rows = list(reader)
    
    updated_rows = []
    
    for row in rows:
        print(f"\nProcessing: {row['Designation']}")
        
        # Calculate total quantity
        try:
            qty = int(row['Quantity']) if row['Quantity'] else 0
            additional = int(row['Additional']) if row['Additional'] else 0
            total_qty = qty + additional
        except ValueError:
            print(f"  ⚠️  Invalid quantity values, skipping")
            updated_rows.append(row)
            continue
        
        # Skip if total is 0 or not a DigiKey URL
        url = row.get('URL', '')
        if total_qty <= 0 or 'digikey.com' not in url:
            print(f"  ⏭️  Skipping (qty={total_qty}, URL={'DigiKey' if 'digikey.com' in url else 'not DigiKey'})")
            updated_rows.append(row)
            continue
        
        # Extract part number from URL
        part_number = api.extract_part_number_from_url(url)
        if not part_number:
            # Try using the Designation as part number
            part_number = row['Designation']
        
        print(f"  📦 Part: {part_number}, Qty: {total_qty}")
        
        # Get product info from DigiKey
        product_info = api.get_product_info(part_number)
        
        if product_info:
            # Extract pricing information
            try:
                # Get manufacturer
                manufacturer = product_info.get('Manufacturer', {})
                if isinstance(manufacturer, dict):
                    manufacturer = manufacturer.get('Name', '')
                else:
                    manufacturer = str(manufacturer)
                
                # Get stock quantity
                stock = product_info.get('QuantityAvailable', 0)
                
                # Get unit price for the quantity needed
                unit_price = 0
                best_packaging = None
                
                # Look through all product variations to find Cut Tape (CT) pricing
                product_variations = product_info.get('ProductVariations', [])
                for variation in product_variations:
                    packaging = variation.get('PackageType', {})
                    packaging_name = packaging.get('Name', '') if isinstance(packaging, dict) else str(packaging)
                    pricing = variation.get('StandardPricing', [])
                    
                    # Prefer Cut Tape or Digi-Reel, but accept any if needed
                    if pricing and ('Cut Tape' in packaging_name or 'Digi-Reel' in packaging_name or best_packaging is None):
                        # Sort pricing by break quantity ascending
                        pricing_sorted = sorted(pricing, key=lambda x: x.get('BreakQuantity', 0))
                        
                        # Find the highest quantity break that is <= our total_qty
                        best_price = None
                        for price_break in pricing_sorted:
                            break_qty = price_break.get('BreakQuantity', 0)
                            if total_qty >= break_qty:
                                best_price = price_break.get('UnitPrice', 0)
                            else:
                                break  # Stop once we exceed our quantity
                        
                        # If we found a matching tier, use it
                        if best_price is not None:
                            unit_price = best_price
                            best_packaging = packaging_name
                            
                            # If this is Cut Tape or Digi-Reel, we're done
                            if 'Cut Tape' in packaging_name or 'Digi-Reel' in packaging_name:
                                break
                
                # If no volume pricing found, use single unit price
                if unit_price == 0:
                    unit_price = product_info.get('UnitPrice', 0)
                
                # Calculate extended price
                ext_price = unit_price * total_qty
                
                # Update row
                row['Manufacturer'] = manufacturer
                row['Stock'] = str(stock)
                row['Unit_Price'] = f"${unit_price:.4f}"
                row['Ext_Price'] = f"${ext_price:.2f}"
                
                print(f"  ✅ Updated: {manufacturer}, Stock: {stock}, Unit: ${unit_price:.4f}, Ext: ${ext_price:.2f}")
                
            except Exception as e:
                print(f"  ❌ Error parsing product info: {e}")
                import traceback
                traceback.print_exc()
        else:
            print(f"  ❌ Could not fetch product info")
        
        updated_rows.append(row)
    
    # Write updated BOM
    with open(output_file, 'w', encoding='utf-8', newline='') as f:
        if updated_rows:
            writer = csv.DictWriter(f, fieldnames=updated_rows[0].keys())  # Default delimiter is comma
            writer.writeheader()
            writer.writerows(updated_rows)
    
    print(f"\n✅ Updated BOM saved to: {output_file}")
    
    # Print summary
    print("\n" + "="*60)
    print("SUMMARY")
    print("="*60)
    total_cost = 0
    for row in updated_rows:
        if row.get('Ext_Price') and row['Ext_Price'].startswith('$'):
            try:
                cost = float(row['Ext_Price'].replace('$', ''))
                total_cost += cost
            except:
                pass
    
    print(f"Total Extended Cost: ${total_cost:.2f}")


def main():
    # Get credentials from file
    credentials_file = 'digikey_credentials.txt'
    
    try:
        with open(credentials_file, 'r') as f:
            lines = f.read().strip().split('\n')
            if len(lines) < 2:
                print(f"ERROR: {credentials_file} must contain:")
                print("  Line 1: Client ID")
                print("  Line 2: Client Secret")
                sys.exit(1)
            
            client_id = lines[0].strip()
            client_secret = lines[1].strip()
            
            if not client_id or not client_secret:
                print("ERROR: Client ID and Secret cannot be empty")
                sys.exit(1)
                
    except FileNotFoundError:
        print(f"ERROR: {credentials_file} not found!")
        print("\nPlease create a file named 'digikey_credentials.txt' with:")
        print("  Line 1: Your DigiKey Client ID")
        print("  Line 2: Your DigiKey Client Secret")
        sys.exit(1)
    
    input_file = 'BOM.csv'  # Adjust if needed
    output_file = 'BOM_updated.csv'  # Adjust if needed
    
    print("DigiKey BOM Updater")
    print("="*60)
    print(f"Input:  {input_file}")
    print(f"Output: {output_file}")
    print("="*60)
    
    # Initialize API
    api = DigiKeyAPI(client_id, client_secret)
    print("🔑 Authenticating with DigiKey API...")
    api.get_access_token()
    print("✅ Authentication successful!\n")
    
    # Update BOM
    update_bom(input_file, output_file, api)


if __name__ == '__main__':
    main()