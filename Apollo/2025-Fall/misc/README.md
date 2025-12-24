# DigiKey BOM Updater

This script automatically fetches pricing, stock, and manufacturer information from the DigiKey API and updates your Bill of Materials (BOM).

## Setup Instructions

### 1. Get DigiKey API Credentials

1. Go to https://developer.digikey.com/
2. Sign up or log in
3. Create a new application to get your:
   - **Client ID**
   - **Client Secret**

### 2. Set Environment Variables

**IMPORTANT**: Never share your API credentials in chat or commit them to version control!

```bash
# On Linux/Mac:
export DIGIKEY_CLIENT_ID='your_client_id_here'
export DIGIKEY_CLIENT_SECRET='your_client_secret_here'

# On Windows (Command Prompt):
set DIGIKEY_CLIENT_ID=your_client_id_here
set DIGIKEY_CLIENT_SECRET=your_client_secret_here

# On Windows (PowerShell):
$env:DIGIKEY_CLIENT_ID='your_client_id_here'
$env:DIGIKEY_CLIENT_SECRET='your_client_secret_here'
```

### 3. Install Dependencies

```bash
pip install requests --break-system-packages
```

### 4. Run the Script

```bash
python3 digikey_bom_updater.py
```

## What It Does

The script will:
- ✅ Skip rows where Quantity + Additional = 0 or less
- ✅ Skip rows with non-DigiKey URLs
- ✅ Extract part numbers from DigiKey URLs
- ✅ Fetch current pricing and stock info
- ✅ Calculate the best unit price based on your total quantity
- ✅ Calculate extended price (Unit Price × Total Quantity)
- ✅ Generate an updated BOM file: `bom_updated.txt`
- ✅ Display a total cost summary

## Output

The script creates `bom_updated.txt` with these columns filled:
- **Manufacturer**: Component manufacturer name
- **Stock**: Available quantity at DigiKey
- **Unit_Price**: Price per unit at your quantity
- **Ext_Price**: Total cost (Unit Price × Quantity)

## Security Notes

🔒 **Keep your credentials safe:**
- Never commit API credentials to git
- Use environment variables (as shown above)
- Don't share credentials in chat or email
- Consider using a `.env` file with proper `.gitignore` rules

## Troubleshooting

**Authentication Failed**: Verify your Client ID and Secret are correct

**Rate Limiting**: DigiKey has API rate limits. The script processes one part at a time to avoid issues.

**Missing Parts**: Some parts may not be found if the URL format is unusual or the part number has changed.

## Example Usage

```bash
# Set credentials (do this once per terminal session)
export DIGIKEY_CLIENT_ID='abc123...'
export DIGIKEY_CLIENT_SECRET='xyz789...'

# Run the updater
python3 digikey_bom_updater.py

# View results
cat bom_updated.txt
```

## Notes

- The script calculates total quantity as: Quantity + Additional
- It uses DigiKey's volume pricing to get the best unit price for your quantity
- Extended price is automatically calculated as: Unit Price × Total Quantity
- The output maintains the same tab-separated format as the input
