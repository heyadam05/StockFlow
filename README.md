# StockFlow

StockFlow is a dependency-free C++17 command-line inventory management system.
It manages products, categories, stock movements, reports, and persistent local
data through a professional command-oriented interface.

## Features

- Product creation, inspection, editing, and deletion
- Validated stock receipts and issues with a permanent transaction history
- Product search by name, description, category, supplier, or barcode
- Filters for category, supplier, quantity, and price
- Category creation, renaming, and safe removal
- Inventory statistics and low-stock warnings
- CSV import and export
- Text report generation
- Automatic persistence, audit logging, and dated backups
- Atomic file replacement to reduce the risk of partial writes
- Automated tests covering core business rules and persistence

## Requirements

- A C++17 compiler (GCC 8+, Clang 7+, or Visual Studio 2019+)
- PowerShell on Windows for the included convenience scripts
- CMake 3.16+ is optional

No third-party C++ libraries are required.

## Quick start on Windows

Open PowerShell in the project directory:

```powershell
cd D:\VisualStudioCode\Projects\StockFlow
.\build.ps1
.\build\stockflow.exe
```

The executable uses the current directory as its data directory. You can pass a
different base directory as the first argument:

```powershell
.\build\stockflow.exe D:\StockFlowData
```

## Build with CMake

```powershell
cmake -S . -B build
cmake --build build --config Release
```

On multi-config generators, the executable will normally be under
`build\Release\stockflow.exe`.

## Run tests

Using the included scripts:

```powershell
.\build.ps1
.\test.ps1
```

Using CMake:

```powershell
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Commands

| Command | Description |
|---|---|
| `help` | Show command reference |
| `add` | Add a product interactively |
| `list` | List all products |
| `show <id>` | Display full product details |
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

Quote arguments containing spaces:

```text
search "MX Master"
categories add "Computer Parts" "Internal and external components"
filter supplier "Example Supplier"
```

Example workflow:

```text
> add
> list
> stock 101 +20 Initial delivery
> stock 101 -3 Customer order
> transactions
> stats
> report
> export exports/inventory.csv
> exit
```

## Data and generated files

StockFlow creates these directories when needed:

- `data/` — product, category, and transaction records plus append-only logs
- `backups/` — dated snapshots created after mutations
- `reports/` — generated inventory reports
- `exports/` — default CSV exports

The `.db` files use a version-friendly escaped text format. Application users
should modify data through StockFlow or CSV import rather than editing these
files manually.

## Architecture

The source is separated into focused layers:

- `Models` defines domain records.
- `Inventory` owns business rules and in-memory state.
- `Storage` handles persistence, backup, logging, and CSV.
- `Application` handles the command-line interface.
- `Utils` contains parsing, formatting, escaping, and date helpers.

This separation keeps business logic independently testable and leaves room for
a future SQLite, GUI, or REST interface without rewriting the inventory rules.

## CSV format

CSV files contain this header:

```text
id,name,description,category,supplier,barcode,price,quantity,minimum,location
```

Imported IDs are intentionally regenerated to avoid collisions. Standard quoted
CSV fields, escaped double quotes, and embedded newlines are supported.