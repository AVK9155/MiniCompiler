import { useState, useEffect, useRef } from "react";
import "./App.css";

const SAMPLES = {
  "Arithmetic": `int x = 10;\nint y = 20;\nint z = x + y * 2;`,
  "Conditional": `int x = 10;\nint y = 20;\nint z = x + y;\nif (z > 25) {\n  int result = z - 5;\n} else {\n  int result = z + 5;\n}`,
  "While loop": `int i = 0;\nint sum = 0;\nwhile (i < 5) {\n  sum = sum + i;\n  i = i + 1;\n}`,
};

const DEFAULT = `// Simplified C — MiniCompiler\nint x = 10;\nint y = 20;\nint z = x + y * 2;\nif (z > 30) {\n  int result = z - 10;\n} else {\n  int result = z + 10;\n}`;

const TABS = ["Output", "Tokens", "Symbols", "IR", "Errors"];

function useNow() {
  const [t, setT] = useState(new Date());
  useEffect(() => { const id = setInterval(() => setT(new Date()), 1000); return () => clearInterval(id); }, []);
  return t.toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
}

export default function App() {
  const [code, setCode]         = useState(DEFAULT);
  const [tab, setTab]           = useState("Output");
  const [result, setResult]     = useState(null);
  const [loading, setLoading]   = useState(false);
  const [elapsed, setElapsed]   = useState(null);
  const [sampleOpen, setSample] = useState(false);
  const sampleRef               = useRef(null);
  const textareaRef             = useRef(null);
  const time                    = useNow();

  const lines = code.split("\n").length;

  useEffect(() => {
    const handler = (e) => {
      if (sampleRef.current && !sampleRef.current.contains(e.target)) setSample(false);
    };
    document.addEventListener("mousedown", handler);
    return () => document.removeEventListener("mousedown", handler);
  }, []);

  const run = async () => {
    if (!code.trim() || loading) return;
    setLoading(true);
    const t0 = Date.now();
    try {
      const res  = await fetch("http://localhost:5000/compile", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ code }),
      });
      const data = await res.json();
      setResult(data);
      setTab(data.errors?.length ? "Errors" : "Output");
    } catch {
      setResult({ tokens: [], symbolTable: [], ir: [], output: "", errors: ["Cannot connect to backend on port 5000."] });
      setTab("Errors");
    }
    setElapsed(Date.now() - t0);
    setLoading(false);
  };

  const clear = () => { setCode(""); setResult(null); setElapsed(null); textareaRef.current?.focus(); };

  const loadSample = (name) => { setCode(SAMPLES[name]); setResult(null); setSample(false); };

  const onKey = (e) => {
    if (e.key === "Tab") {
      e.preventDefault();
      const s = e.target.selectionStart;
      const v = code.substring(0, s) + "  " + code.substring(e.target.selectionEnd);
      setCode(v);
      setTimeout(() => { e.target.selectionStart = e.target.selectionEnd = s + 2; }, 0);
    }
    if ((e.ctrlKey || e.metaKey) && e.key === "Enter") run();
  };

  const hasErr   = result?.errors?.length > 0;
  const tokCount = result?.tokens?.length ?? 0;
  const symCount = result?.symbolTable?.length ?? 0;

  return (
    <div className="app">

      {/* HEADER */}
      <header className="header">
        <span className="header-title">MiniCompiler</span>
        <span className="header-sep">|</span>
        <span className="header-file">main.mc</span>
        <div className="header-actions">
          <div className="sample-wrap" ref={sampleRef}>
            <button className="btn" onClick={() => setSample(v => !v)}>Samples ▾</button>
            {sampleOpen && (
              <div className="sample-menu">
                {Object.keys(SAMPLES).map(k => (
                  <button key={k} className="sample-item" onClick={() => loadSample(k)}>{k}</button>
                ))}
              </div>
            )}
          </div>
          <button className="btn" onClick={clear}>Clear</button>
          <button className="btn btn-primary" onClick={run} disabled={loading}>
            {loading ? <span className="spinner" /> : "Run  ⌘↵"}
          </button>
        </div>
      </header>

      {/* MAIN */}
      <div className="main">

        {/* EDITOR */}
        <div className="editor-side">
          <div className="editor-wrap">
            <div className="line-nums">
              {Array.from({ length: lines }, (_, i) => (
                <div key={i}>{i + 1}</div>
              ))}
            </div>
            <textarea
              ref={textareaRef}
              className="editor"
              value={code}
              onChange={e => setCode(e.target.value)}
              onKeyDown={onKey}
              placeholder="// write your simplified C code here..."
              spellCheck={false}
              autoComplete="off"
              autoCorrect="off"
              autoCapitalize="off"
            />
          </div>
        </div>

        {/* OUTPUT */}
        <div className="output-side">
          <div className="output-header">
            {TABS.map(t => (
              <button
                key={t}
                className={`out-tab${tab === t ? " active" : ""}`}
                onClick={() => setTab(t)}
              >
                {t}
                {t === "Errors" && hasErr && <span className="err-dot" />}
              </button>
            ))}
          </div>

          <div className="output-body">

            {/* OUTPUT TAB */}
            {tab === "Output" && (
              !result
                ? <div className="empty">— run to see output —</div>
                : !result.output
                  ? <div className="empty">no output</div>
                  : <>
                      {result.output.split("\n").filter(Boolean).map((l, i) => (
                        <div key={i} className="result-line">{l}</div>
                      ))}
                      {elapsed && <div className="result-note">completed in {elapsed}ms</div>}
                    </>
            )}

            {/* TOKENS TAB */}
            {tab === "Tokens" && (
              !result
                ? <div className="empty">— run to tokenize —</div>
                : tokCount === 0
                  ? <div className="empty">no tokens</div>
                  : <table className="out-table">
                      <thead><tr><th>#</th><th>Value</th><th>Type</th></tr></thead>
                      <tbody>
                        {result.tokens.map((tok, i) => (
                          <tr key={i}>
                            <td>{i + 1}</td>
                            <td>{tok.value ?? tok.token ?? String(tok)}</td>
                            <td>{tok.type ?? "—"}</td>
                          </tr>
                        ))}
                      </tbody>
                    </table>
            )}

            {/* SYMBOLS TAB */}
            {tab === "Symbols" && (
              !result
                ? <div className="empty">— run to analyze —</div>
                : !result.symbolTable?.length
                  ? <div className="empty">no symbols</div>
                  : <table className="out-table">
                      <thead><tr><th>Name</th><th>Type</th><th>Value</th></tr></thead>
                      <tbody>
                        {result.symbolTable.map((s, i) => (
                          <tr key={i}>
                            <td>{s.name ?? String(s)}</td>
                            <td>{s.type ?? "int"}</td>
                            <td>{s.value ?? "—"}</td>
                          </tr>
                        ))}
                      </tbody>
                    </table>
            )}

            {/* IR TAB */}
            {tab === "IR" && (
              !result
                ? <div className="empty">— run to generate IR —</div>
                : !result.ir?.length
                  ? <div className="empty">no IR generated</div>
                  : <div className="ir-block">
                      {result.ir.map((line, i) => {
                        const str = typeof line === "string" ? line : JSON.stringify(line);
                        const isLabel = str.trim().endsWith(":");
                        return (
                          <div key={i} className={isLabel ? "ir-label" : "ir-line"}>
                            {str}
                          </div>
                        );
                      })}
                    </div>
            )}

            {/* ERRORS TAB */}
            {tab === "Errors" && (
              !result
                ? <div className="empty">— run to check errors —</div>
                : !hasErr
                  ? <div className="no-err">✓ no errors</div>
                  : result.errors.map((e, i) => (
                      <div key={i} className="err-item">
                        {typeof e === "string" ? e : JSON.stringify(e)}
                      </div>
                    ))
            )}

          </div>
        </div>
      </div>

      {/* STATUS BAR */}
      <footer className="statusbar">
        <span className="status-item">
          {loading
            ? "compiling..."
            : result
              ? hasErr
                ? <span className="status-err">✗ {result.errors.length} error{result.errors.length > 1 ? "s" : ""}</span>
                : <span className="status-ok">✓ ok</span>
              : "ready"}
        </span>
        <span className="status-item">ln {lines}</span>
        {result && <span className="status-item">{tokCount} tokens</span>}
        {result && <span className="status-item">{symCount} symbols</span>}
        {elapsed && <span className="status-item">{elapsed}ms</span>}
        <span className="status-item status-ml">{time}</span>
      </footer>

    </div>
  );
}
