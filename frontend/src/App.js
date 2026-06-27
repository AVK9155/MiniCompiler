import { useState, useEffect, useRef } from "react";
import "./App.css";

const SAMPLE_PROGRAMS = {
  arithmetic: `// Arithmetic sample
int x = 10;
int y = 20;
int z = x + y * 2;`,

  condition: `// Conditional sample
int x = 10;
int y = 20;
int z = x + y;
if (z > 25) {
  int result = z - 5;
} else {
  int result = z + 5;
}`,

  loop: `// While loop sample
int i = 0;
int sum = 0;
while (i < 5) {
  sum = sum + i;
  i = i + 1;
}`,
};

const DEFAULT_CODE = `// MiniCompiler - Simplified C
int x = 10;
int y = 20;
int z = x + y * 2;
if (z > 30) {
  int result = z - 10;
} else {
  int result = z + 10;
}`;

function useTime() {
  const [time, setTime] = useState(new Date());
  useEffect(() => {
    const t = setInterval(() => setTime(new Date()), 1000);
    return () => clearInterval(t);
  }, []);
  return time.toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", second: "2-digit" });
}

export default function App() {
  const [code, setCode] = useState(DEFAULT_CODE);
  const [activeTab, setActiveTab] = useState("tokens");
  const [result, setResult] = useState(null);
  const [loading, setLoading] = useState(false);
  const [compileTime, setCompileTime] = useState(null);
  const [showSampleMenu, setShowSampleMenu] = useState(false);
  const time = useTime();
  const textareaRef = useRef(null);
  const sampleMenuRef = useRef(null);

  const lineCount = code.split("\n").length;

  // Close sample menu on outside click
  useEffect(() => {
    function handleClick(e) {
      if (sampleMenuRef.current && !sampleMenuRef.current.contains(e.target)) {
        setShowSampleMenu(false);
      }
    }
    document.addEventListener("mousedown", handleClick);
    return () => document.removeEventListener("mousedown", handleClick);
  }, []);

  const handleCompile = async () => {
    if (!code.trim()) return;
    setLoading(true);
    const start = Date.now();
    try {
      const res = await fetch("http://localhost:5000/compile", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ code }),
      });
      const data = await res.json();
      setResult(data);
      setCompileTime(Date.now() - start);
      setActiveTab(data.errors && data.errors.length > 0 ? "errors" : "tokens");
    } catch (err) {
      setResult({
        tokens: [],
        symbolTable: [],
        ir: [],
        output: "",
        errors: ["ERROR: Cannot connect to backend. Is the server running on port 5000?"],
      });
      setCompileTime(Date.now() - start);
      setActiveTab("errors");
    }
    setLoading(false);
  };

  const handleClear = () => {
    setCode("");
    setResult(null);
    setCompileTime(null);
    textareaRef.current?.focus();
  };

  const loadSample = (key) => {
    setCode(SAMPLE_PROGRAMS[key]);
    setResult(null);
    setShowSampleMenu(false);
    textareaRef.current?.focus();
  };

  const hasErrors = result?.errors?.length > 0;
  const tokenCount = result?.tokens?.length ?? 0;
  const symbolCount = result?.symbolTable?.length ?? 0;

  return (
    <div className="dos-window">

      {/* ── MENU BAR ── */}
      <div className="menubar">
        <button className="menu-item"><span className="underline">F</span>ile</button>
        <button className="menu-item"><span className="underline">E</span>dit</button>
        <button className="menu-item" onClick={handleCompile}><span className="underline">C</span>ompile</button>
        <button className="menu-item"><span className="underline">V</span>iew</button>
        <button className="menu-item"><span className="underline">H</span>elp</button>
        <div className="menu-spacer" />
        <span className="menu-clock">{time}</span>
      </div>

      {/* ── TITLE BAR ── */}
      <div className="titlebar">
        ■ MINICOMPILER v1.0 — SIMPLIFIED C LANGUAGE IDE ■
      </div>

      {/* ── TOOLBAR ── */}
      <div className="toolbar">
        <button className="dos-btn primary" onClick={handleCompile} disabled={loading}>
          {loading ? "► COMPILING..." : "► COMPILE [F5]"}
        </button>

        <span className="toolbar-sep">│</span>

        <button className="dos-btn" onClick={handleClear}>
          CLR
        </button>

        <div style={{ position: "relative" }} ref={sampleMenuRef}>
          <button className="dos-btn" onClick={() => setShowSampleMenu(v => !v)}>
            SAMPLES ▼
          </button>
          {showSampleMenu && (
            <div style={{
              position: "absolute",
              top: "100%",
              left: 0,
              background: "#aaaaaa",
              border: "2px solid #555",
              zIndex: 100,
              minWidth: "160px",
              fontFamily: "var(--font-dos)",
            }}>
              {Object.entries({ arithmetic: "Arithmetic", condition: "Conditional", loop: "While Loop" }).map(([k, label]) => (
                <button
                  key={k}
                  onClick={() => loadSample(k)}
                  style={{
                    display: "block", width: "100%", textAlign: "left",
                    padding: "3px 12px", background: "transparent",
                    border: "none", fontFamily: "var(--font-dos)",
                    fontSize: "16px", color: "#000", cursor: "pointer",
                  }}
                  onMouseEnter={e => { e.target.style.background = "#000080"; e.target.style.color = "#fff"; }}
                  onMouseLeave={e => { e.target.style.background = "transparent"; e.target.style.color = "#000"; }}
                >
                  {label}
                </button>
              ))}
            </div>
          )}
        </div>

        <span className="toolbar-label">
          LN:{lineCount} │ {code.length} BYTES │ {loading ? "COMPILING..." : result ? (hasErrors ? "ERRORS" : "READY") : "IDLE"}
        </span>
      </div>

      {/* ── MAIN BODY ── */}
      <div className="dos-body">

        {/* ── EDITOR ── */}
        <div className="editor-panel">
          <div className="panel-titlebar">
            ▶ SOURCE EDITOR
            <span className="modified">{result ? "" : code !== DEFAULT_CODE ? " [MODIFIED]" : ""}</span>
          </div>
          <div className="editor-wrapper">
            <div className="line-numbers">
              {Array.from({ length: lineCount }, (_, i) => (
                <div key={i}>{String(i + 1).padStart(2, "0")}</div>
              ))}
            </div>
            <textarea
              ref={textareaRef}
              className="code-editor"
              value={code}
              onChange={e => setCode(e.target.value)}
              onKeyDown={e => {
                if (e.key === "Tab") {
                  e.preventDefault();
                  const s = e.target.selectionStart;
                  const end = e.target.selectionEnd;
                  const newVal = code.substring(0, s) + "  " + code.substring(end);
                  setCode(newVal);
                  setTimeout(() => { e.target.selectionStart = e.target.selectionEnd = s + 2; }, 0);
                }
                if (e.key === "F5") { e.preventDefault(); handleCompile(); }
              }}
              placeholder="// Enter your simplified C code here..."
              spellCheck={false}
              autoComplete="off"
              autoCorrect="off"
              autoCapitalize="off"
            />
          </div>
        </div>

        {/* ── RIGHT PANEL ── */}
        <div className="right-panel">
          <div className="out-tabs">
            {["tokens", "symbols", "ir", "output", "errors"].map(tab => (
              <button
                key={tab}
                className={`out-tab${activeTab === tab ? " active" : ""}`}
                onClick={() => setActiveTab(tab)}
              >
                {tab === "tokens" ? "TOK" :
                 tab === "symbols" ? "SYM" :
                 tab === "ir" ? "IR" :
                 tab === "output" ? "OUT" : "ERR"}
                {tab === "errors" && hasErrors && (
                  <span style={{ color: "var(--dos-red)", marginLeft: 2 }}>!</span>
                )}
              </button>
            ))}
          </div>

          <div className="out-content">
            {activeTab === "tokens" && (
              !result ? (
                <div className="empty-state">► Press COMPILE to tokenize...</div>
              ) : result.tokens.length === 0 ? (
                <div className="empty-state">No tokens found.</div>
              ) : (
                <table className="token-table">
                  <thead>
                    <tr>
                      <th>#</th>
                      <th>VALUE</th>
                      <th>TYPE</th>
                    </tr>
                  </thead>
                  <tbody>
                    {result.tokens.map((tok, i) => (
                      <tr key={i}>
                        <td style={{ color: "var(--dos-darkgray)", fontSize: 13 }}>{String(i + 1).padStart(2, "0")}</td>
                        <td className="val">{tok.value ?? tok.token ?? tok}</td>
                        <td className="type">{tok.type ?? "—"}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              )
            )}

            {activeTab === "symbols" && (
              !result ? (
                <div className="empty-state">► Press COMPILE to analyze...</div>
              ) : !result.symbolTable || result.symbolTable.length === 0 ? (
                <div className="empty-state">No symbols found.</div>
              ) : (
                <table className="sym-table">
                  <thead>
                    <tr>
                      <th>NAME</th>
                      <th>TYPE</th>
                      <th>VALUE</th>
                    </tr>
                  </thead>
                  <tbody>
                    {result.symbolTable.map((sym, i) => (
                      <tr key={i}>
                        <td className="name">{sym.name ?? sym.identifier ?? sym}</td>
                        <td className="type">{sym.type ?? "int"}</td>
                        <td className="val">{sym.value ?? "—"}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              )
            )}

            {activeTab === "ir" && (
              !result ? (
                <div className="empty-state">► Press COMPILE to generate IR...</div>
              ) : !result.ir || result.ir.length === 0 ? (
                <div className="empty-state">No IR generated.</div>
              ) : (
                <div>
                  {result.ir.map((line, i) => (
                    <div key={i} className="ir-line">
                      <span style={{ color: "var(--dos-darkgray)", marginRight: 8, fontSize: 13 }}>
                        {String(i + 1).padStart(2, "0")}
                      </span>
                      {typeof line === "string" ? line : JSON.stringify(line)}
                    </div>
                  ))}
                </div>
              )
            )}

            {activeTab === "output" && (
              !result ? (
                <div className="empty-state">► Press COMPILE to run...</div>
              ) : !result.output ? (
                <div className="empty-state">No output produced.</div>
              ) : (
                <div>
                  {result.output.split("\n").map((line, i) => (
                    <div key={i} className="out-result-line">
                      <span className="out-prompt">C:\&gt;</span>{line}
                    </div>
                  ))}
                  <div className="out-result-line" style={{ marginTop: 8, color: "var(--dos-darkgray)" }}>
                    — EXECUTION COMPLETE {compileTime ? `[${compileTime}ms]` : ""} —
                  </div>
                </div>
              )
            )}

            {activeTab === "errors" && (
              !result ? (
                <div className="empty-state">► Press COMPILE to check errors...</div>
              ) : !hasErrors ? (
                <div className="no-errors">✓ NO ERRORS DETECTED</div>
              ) : (
                <div>
                  {result.errors.map((err, i) => (
                    <div key={i} className="err-line">
                      ► {typeof err === "string" ? err : JSON.stringify(err)}
                    </div>
                  ))}
                </div>
              )
            )}
          </div>
        </div>
      </div>

      {/* ── STATUS BAR ── */}
      <div className="statusbar">
        <span className="status-item">
          {result
            ? hasErrors
              ? <span className="status-err">✗ COMPILE FAILED</span>
              : <span className="status-ok">✓ COMPILE OK</span>
            : "READY"
          }
        </span>
        <span className="status-sep">│</span>
        <span className="status-item">TOKENS: {tokenCount}</span>
        <span className="status-sep">│</span>
        <span className="status-item">SYMBOLS: {symbolCount}</span>
        <span className="status-sep">│</span>
        <span className="status-item">LINES: {lineCount}</span>
        {compileTime && (
          <>
            <span className="status-sep">│</span>
            <span className="status-item">TIME: {compileTime}ms</span>
          </>
        )}
        <span className="status-sep" style={{ marginLeft: "auto" }}>│</span>
        <span className="status-item">SIMPLIFIED-C │ UTF-8 │ CRLF</span>
      </div>

    </div>
  );
}
