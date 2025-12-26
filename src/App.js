import { useState } from "react";

function App() {
  const [code, setCode] = useState("");
  const [output, setOutput] = useState("");

  const compileCode = async () => {
    try {
      const response = await fetch("http://localhost:5000/compile", {
        method: "POST",
        headers: {
          "Content-Type": "application/json"
        },
        body: JSON.stringify({ code })
      });

      const data = await response.json();
      setOutput(
  "TOKENS:\n" +
  JSON.stringify(data.tokens, null, 2) +
  "\n\nSYMBOL TABLE:\n" +
  JSON.stringify(data.symbolTable, null, 2)
);

    } catch (error) {
      console.error(error);
      setOutput("Error connecting to backend");
    }
  };

  return (
    <div style={{ padding: "20px" }}>
      <h1>Mini Compiler</h1>

      <textarea
        rows="10"
        cols="80"
        value={code}
        onChange={(e) => setCode(e.target.value)}
        placeholder="Write your C code here..."
      />

      <br /><br />

      <button onClick={compileCode}>Compile</button>

      <pre>{output}</pre>
    </div>
  );
}

export default App;
