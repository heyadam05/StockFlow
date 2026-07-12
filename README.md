# StockFlow

StockFlow is a dependency-free C++17 command-line inventory manager. It keeps products, categories, stock movements, reports, imports, exports, backups, and audit logs in a clean local workflow.

## Highlights

- Manage products with descriptions, categories, suppliers, barcodes, prices, and quantities.
- Receive and issue stock with a permanent transaction history.
- Search by name, description, category, supplier, or barcode.
- Filter by category, supplier, quantity, and price.
- Manage categories safely, including rename and unused-category removal.
- Generate inventory statistics, low-stock warnings, text reports, and CSV exports.
- Persist data locally with dated backups and atomic file replacement.
- Run automated tests for business rules and persistence.

## Requirements

- C++17 compiler: GCC 8+, Clang 7+, or Visual Studio 2019+
- PowerShell on Windows for the helper scripts
- CMake 3.16+ if you prefer a CMake build

No third-party C++ libraries are required.

## Run

From PowerShell in the project directory:

```powershell
cd D:\VisualStudioCode\Projects\StockFlow
.\build.ps1
.\build\stockflow.exe
```

The executable uses the current directory as its data directory. You can pass another directory as the first argument:

```powershell
.\build\stockflow.exe D:\StockFlowData
```

## Build With CMake

```powershell
cmake -S . -B build
cmake --build build --config Release
```

With multi-config generators, the executable is usually created at `build\Release\stockflow.exe`.

## Test

```powershell
.\build.ps1
.\test.ps1
```

Or with CMake:

```powershell
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Commands

| Command | Description |
| --- | --- |
| `help` | Show command reference |
| `add` | Add a product interactively |
| `list` | List all products |
| `show <id>` | Display product details |
| `edit <id>` | Edit a product interactively |
| `delete <id>` | Delete a product after confirmation |
| `stock <id> <+/-amount> [note]` | Receive or issue stock |
| `search <text>` | Search product fields |
| `filter category <value>` | Filter by category |
| `filter supplier <value>` | Filter by supplier |
| `filter price <operator> <value>` | Filter by price |
| `filter quantity <operator> <value>` | Filter by quantity |
| `categories list` | List categories |
| `categories add <name> [description]` | Add a category |
| `categories rename <old> <new>` | Rename a category |
| `categories remove <name>` | Remove an unused category |
| `transactions` | Show stock movement history |
| `stats` | Show inventory statistics |
| `report` | Generate a report |
| `export [path.csv]` | Export products to CSV |
| `import <path.csv>` | Import products from CSV |
| `clear` | Clear the terminal |
| `exit` | Exit the application |

Quote arguments that contain spaces:

```text
search "MX Master"
categories add "Computer Parts" "Internal and external components"
filter supplier "Example Supplier"
```

## Data And Files

StockFlow creates these directories as needed:

- `data/` for products, categories, transactions, and append-only logs
- `backups/` for dated snapshots
- `reports/` for generated inventory reports
- `exports/` for default CSV exports

The `.db` files use a version-friendly escaped text format. Use StockFlow or CSV import to change data instead of editing these files manually.
