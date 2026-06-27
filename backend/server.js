const express = require("express");
const cors = require("cors");

const app = express();
const PORT = 5000;

app.use(cors());
app.use(express.json());

/* ================================
   COMPILE API
================================ */
app.post("/compile", (req, res) => {
  const code = req.body.code || "";

  let tokens = [];
  let symbolTable = {};
  let syntaxErrors = [];
  let semanticErrors = [];
  let typeErrors = [];
  let intermediateCode = [];

  const keywords = ["int"];
  let tempCount = 1;

  const lines = code.split("\n");

  /* ================================
     LEXICAL + BASIC SYNTAX
  ================================ */
  lines.forEach((line, lineNumber) => {
    line = line.trim();
    if (line === "") return;

    if (!line.endsWith(";")) {
      syntaxErrors.push(
        `Syntax Error (Line ${lineNumber + 1}): Missing semicolon`
      );
    }

    const clean = line.replace(";", "");

    // Handle string
    if (clean.includes('"')) {
      const match = clean.match(/"(.*?)"/);
      if (match) {
        tokens.push({ type: "STRING", value: match[0] });
        tokens.push({ type: "SYMBOL", value: ";" });
        return;
      }
    }

    const parts = clean.split(/\s+/);

    parts.forEach((part) => {
      if (keywords.includes(part)) {
        tokens.push({ type: "KEYWORD", value: part });
      } else if (!isNaN(part)) {
        tokens.push({ type: "NUMBER", value: part });
      } else if (part === "=" || part === "+" || part === "-") {
        tokens.push({ type: "OPERATOR", value: part });
      } else if (part.includes("=")) {
        const split = part.split("=");
        tokens.push({ type: "IDENTIFIER", value: split[0] });
        tokens.push({ type: "OPERATOR", value: "=" });
        if (!isNaN(split[1])) {
          tokens.push({ type: "NUMBER", value: split[1] });
        }
      } else {
        tokens.push({ type: "IDENTIFIER", value: part });
      }
    });

    tokens.push({ type: "SYMBOL", value: ";" });
  });

  /* ================================
     SYMBOL + SEMANTIC + TYPE + ICG
  ================================ */
  for (let i = 0; i < tokens.length; i++) {

    // Declaration
    if (tokens[i].type === "KEYWORD" && tokens[i].value === "int") {
      const varName = tokens[i + 1]?.value;

      if (!varName) {
        syntaxErrors.push("Syntax Error: Invalid declaration");
        continue;
      }

      if (symbolTable[varName]) {
        semanticErrors.push(
          `Semantic Error: Variable '${varName}' already declared`
        );
      } else {
        symbolTable[varName] = { type: "int" };
      }
    }

    // Assignment with expression
    if (
      tokens[i].type === "IDENTIFIER" &&
      tokens[i + 1]?.value === "="
    ) {
      const varName = tokens[i].value;

      if (!symbolTable[varName]) {
        semanticErrors.push(
          `Semantic Error: Variable '${varName}' not declared`
        );
        continue;
      }

      const op1 = tokens[i + 2];
      const operator = tokens[i + 3];
      const op2 = tokens[i + 4];

      // Simple assignment: a = 10;
      if (op1 && op1.type === "NUMBER" && !operator) {
        intermediateCode.push(`${varName} = ${op1.value}`);
      }

      // Expression: a = 10 + 20;
      if (
        op1?.type === "NUMBER" &&
        operator?.type === "OPERATOR" &&
        op2?.type === "NUMBER"
      ) {
        const temp = `t${tempCount++}`;
        intermediateCode.push(
          `${temp} = ${op1.value} ${operator.value} ${op2.value}`
        );
        intermediateCode.push(`${varName} = ${temp}`);
      }

      // Type check
      if (op1?.type === "STRING") {
        typeErrors.push(
          `Type Error: Cannot assign string to int variable '${varName}'`
        );
      }
    }
  }

  /* ================================
     RESPONSE
  ================================ */
  res.json({
    tokens,
    symbolTable: Object.entries(symbolTable).map(([name, info]) => ({
      name,
      type: info.type,
    })),
    syntaxErrors,
    semanticErrors,
    typeErrors,
    intermediateCode,
  });
});

/* ================================
   SERVER START
================================ */
app.listen(PORT, () => {
  console.log(`Mini Compiler Backend running on port ${PORT}`);
});
