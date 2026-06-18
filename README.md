# 🔧 MiniCompiler

A full-stack mini compiler that processes a simplified C-like language through all major compilation phases — lexical analysis, parsing, semantic analysis, intermediate representation, and interpretation — with a live web interface.

> Built with React (frontend) · Node.js/Express (backend) · JavaScript (compiler logic) · C (compiler stub layer)

[![GitHub](https://img.shields.io/badge/GitHub-AVK9155%2FMiniCompiler-blue?logo=github)](https://github.com/AVK9155/MiniCompiler)
![Language](https://img.shields.io/badge/Language-C--like-orange)
![Stack](https://img.shields.io/badge/Stack-React%20%7C%20Node.js%20%7C%20C-green)

---

## 📸 Features

- ✅ **Lexical Analysis** — Tokenizes input into keywords, identifiers, operators, literals
- ✅ **Parsing** — Builds a parse tree from token stream
- ✅ **Semantic Analysis** — Type checking, undeclared variable detection
- ✅ **Intermediate Representation (IR)** — Three-address code generation
- ✅ **Interpretation** — Executes the IR and produces output
- ✅ **Symbol Table** — Tracks variable declarations and types
- ✅ **Web Interface** — Write, compile, and debug code in the browser

---

## 🏗️ Architecture

```
MiniCompiler/
├── frontend/          # React app — code editor + output display
│   ├── src/
│   └── public/
├── backend/           # Node.js/Express — API server
│   └── server.js
├── compiler/          # C source files — compiler stub layer
│   ├── lexer.c
│   ├── parser.c
│   ├── semantic.c
│   ├── ir.c
│   ├── interpreter.c
│   ├── symbol_table.c
│   ├── main.c
│   └── compiler.h
├── samples/           # Sample programs in the mini-language
└── report/            # Project documentation
```

### Data Flow

```
User Code (Browser)
      ↓
  React Frontend
      ↓  (HTTP POST /compile)
  Express Backend
      ↓
  Compiler Logic (JS)
  ┌─────────────────────────┐
  │  Lexer → Tokens         │
  │  Parser → Parse Tree    │
  │  Semantic → Checked AST │
  │  IR Generator → TAC     │
  │  Interpreter → Output   │
  └─────────────────────────┘
      ↓
  JSON Response
      ↓
  React Frontend (Tokens Table, Symbol Table, IR, Output)
```

---

## 🚀 Getting Started

### Prerequisites

- [Node.js](https://nodejs.org/) v16+
- npm v8+
- GCC (optional — for C layer compilation)

### 1. Clone the Repository

```bash
git clone https://github.com/AVK9155/MiniCompiler.git
cd MiniCompiler
```

### 2. Start the Backend

```bash
cd backend
npm install
node server.js
```

> Backend runs on `http://localhost:5000`

### 3. Start the Frontend

```bash
cd frontend
npm install
npm start
```

> Frontend runs on `http://localhost:3000`

### 4. (Optional) Compile the C Layer

```bash
cd compiler
gcc -o minicompiler main.c lexer.c parser.c semantic.c ir.c interpreter.c symbol_table.c
./minicompiler
```

---

## 📝 Supported Language Syntax

The compiler supports a simplified C-like language:

```c
// Variable declarations
int x = 10;
int y = 20;

// Arithmetic
int z = x + y * 2;

// If-else
if (z > 30) {
    int result = z - 10;
} else {
    int result = z + 10;
}

// While loop
int i = 0;
while (i < 5) {
    i = i + 1;
}
```

### Supported Features

| Feature | Status |
|---|---|
| `int` declarations | ✅ |
| Arithmetic (`+`, `-`, `*`, `/`) | ✅ |
| Comparison operators | ✅ |
| `if` / `else` | ✅ |
| `while` loop | ✅ |
| Nested expressions | ✅ |
| Semantic error detection | ✅ |
| `float` type | 🔄 Planned |
| Functions | 🔄 Planned |
| Arrays | 🔄 Planned |

---

## 🖥️ API Reference

### `POST /compile`

Compiles the provided source code and returns all compilation phase outputs.

**Request Body:**
```json
{
  "code": "int x = 5;\nint y = x + 3;"
}
```

**Response:**
```json
{
  "tokens": [...],
  "symbolTable": [...],
  "ir": [...],
  "output": "...",
  "errors": []
}
```

---

## 📂 Sample Programs

Sample programs are in the `/samples` folder:

| File | Description |
|---|---|
| `hello.c` | Basic variable declaration and arithmetic |
| `loop.c` | While loop example |
| `conditions.c` | If-else branching |

---

## 🛠️ Tech Stack

| Layer | Technology |
|---|---|
| Frontend | React, HTML, CSS, JavaScript |
| Backend | Node.js, Express.js |
| Compiler Logic | JavaScript |
| C Stub Layer | GCC / C99 |

---

## 👨‍💻 Author

**Damu Balaji** — Third-year B.Tech CS student at KL University  

[![GitHub](https://img.shields.io/badge/GitHub-AVK9155-black?logo=github)](https://github.com/AVK9155)

---

## 📄 License

This project is for educational purposes as part of a compiler design course.
