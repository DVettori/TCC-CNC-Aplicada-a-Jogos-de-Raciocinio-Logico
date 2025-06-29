//Autores: Daniel Oliveira Vettorazzi e Fabio Miranda Machado Junior
//Engenharia de controle e Automação
#include <WiFi.h>
#include <WebServer.h>

#define RX2 16  // Comunicação com CNC (GRBL)
#define TX2 17

const char* ssid = "UCL_CNC";
const char* password = "12345678";

const char* positions[] = {
  "G90 X4 Y7\n",   // 0
  "G90 X4 Y14\n",  // 1
  "G90 X4 Y21\n",  // 2
  "G90 X11 Y7\n",  // 3
  "G90 X11 Y14\n", // 4
  "G90 X11 Y21\n", // 5
  "G90 X18 Y7\n",  // 6
  "G90 X18 Y14\n", // 7
  "G90 X18 Y21\n"  // 8
};

// LED códigos para Mega
const char ledCodes[9][2] = {
  {'m', 'n'}, // 0
  {'o', 'p'}, // 1
  {'q', 'r'}, // 2
  {'g', 'h'}, // 3
  {'i', 'j'}, // 4
  {'k', 'l'}, // 5
  {'a', 'b'}, // 6
  {'c', 'd'}, // 7
  {'e', 'f'}  // 8
};

WebServer server(80);

// Controle de travamento até pressionar botão físico correto
volatile bool aguardandoBotao = false;
volatile int ultimaPosicao = -1;
volatile char ultimoPlayer = 'X';

String htmlPage = R"rawliteral(

<!DOCTYPE html>
<html lang="pt-br">
<head>
  <meta charset="UTF-8">
  <title>Jogo da Velha CNC</title>
  <style>
    body {margin:0;padding:0;background:#f4f4f4;font-family:'Segoe UI',Arial,sans-serif;min-height:100vh;}
    .header {display:flex;align-items:center;justify-content:flex-start;background:#fff;box-shadow:0 2px 6px rgba(0,0,0,.07);position:relative;min-height:140px;padding-left:24px;}
    .ucl-logo {display:flex;flex-direction:column;align-items:flex-start;justify-content:center;margin-right:30px;user-select:none;font-family:'Segoe UI',Arial,sans-serif;letter-spacing:1.5px;min-width:115px;}
    .ucl-logo .ucl-top {font-size:1.07em;color:#185fb8;font-weight:700;margin-bottom:-2px;letter-spacing:2px;line-height:1;}
    .ucl-logo .ucl-bottom {font-size:3.6em;color:#185fb8;font-weight:900;letter-spacing:2px;line-height:1;}
    .title-center-area {flex:1;display:flex;justify-content:center;align-items:center;min-height:88px;position:absolute;left:0;right:0;pointer-events:none;}
    .title {font-size:2em;font-weight:600;color:#222;text-align:center;line-height:1.1;width:100%;pointer-events:none;}
    #modeButtons {position:absolute;top:38px;right:42px;display:grid;grid-template-columns:1fr 1fr;grid-template-rows:1fr 1fr;gap:8px;z-index:10;text-align:right;min-width:190px;}
    #modeButtons button {padding:7.5px 0;font-size:0.75em;border-radius:6px;border:1px solid #aaa;background:#fafbfc;transition:background 0.2s,box-shadow 0.2s;box-shadow:0 1px 2px rgba(0,0,0,.04);width:90px;margin:0 auto;display:block;cursor:pointer;}
    #modeButtons button:hover {background:#e8e9ea;}
    .score-container {display:flex;flex-direction:column;align-items:center;margin:0 auto 20px;}
    .score-card {background:#fff;border-radius:12px;box-shadow:0 2px 6px rgba(0,0,0,.09);padding:12px 36px 10px;margin-top:8px;min-width:200px;text-align:center;}
    .score-card-title {font-weight:bold;font-size:1.15em;margin-bottom:4px;color:#222;letter-spacing:0.5px;}
    .score-bar {display:flex;justify-content:center;gap:18px;align-items:center;margin-bottom:3px;}
    .score-x, .score-o {border-radius:7px;padding:6px 22px;font-size:1.18em;font-weight:700;color:#fff;display:inline-block;min-width:38px;}
    .score-x {background:#4a90e2;}
    .score-o {background:#e74c3c;}
    #starterDisplay {color:#222;font-size:1em;margin-top:3px;margin-bottom:2px;}
    .main {width:100%;min-height:70vh;display:flex;flex-direction:column;align-items:center;justify-content:flex-start;padding:26px 0 0;}
    #gameBoard {background:#fff;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,.07);margin:0 auto 20px;}
    table {border-collapse:collapse;margin:0 auto;}
    td {width:88px;height:88px;border:2px solid #333;cursor:pointer;transition:background-color 0.2s,transform 0.15s;font-size:2.3em;text-align:center;vertical-align:middle;background:#fff;}
    td.player {background:#4a90e2!important;}
    td.machine {background:#e74c3c!important;}
    td:hover {transform:scale(1.04);}
    #message {font-size:1.12em;margin:13px;min-height:32px;text-align:center;color:#222;}
    #logBox {position:fixed;bottom:32px;right:32px;width:300px;max-height:300px;background:#232323;border-radius:16px;box-shadow:0 3px 18px rgba(0,0,0,0.25);color:#fff;padding:0;z-index:20;display:flex;flex-direction:column;overflow:hidden;font-size:1em;}
    #logTitle {background:#232323;padding:13px 16px 9px;font-weight:bold;font-size:1.06em;border-bottom:1px solid #333;letter-spacing:.2px;}
    #logContent {padding:9px 16px 13px;overflow-y:auto;flex:1;font-size:.97em;line-height:1.5;}
    .authors {position:fixed;left:12px;bottom:5px;font-size:.94em;color:#888;z-index:30;letter-spacing:.15px;}
    @media (max-width:767px) {
      .header {min-height:80px;padding:0 0 0 8px;}
      .ucl-logo .ucl-bottom {font-size:1.7em;}
      .title {font-size:1em;}
      .title-center-area {min-height:54px;}
      #modeButtons {right:2px;top:7px;min-width:80px;}
      #modeButtons button {font-size:.69em;width:58px;}
      .score-card {padding:8px 8px 7px 8px;}
      td {width:54px;height:54px;font-size:1.15em;}
      #logBox {right:4px;bottom:4px;width:97vw;max-width:99vw;min-width:0;font-size:.92em;}
      .authors {left:4px;bottom:2px;font-size:.80em;}
    }
  </style>
</head>
<body>
  <div class="header">
    <div class="ucl-logo">
      <span class="ucl-top">FACULDADE</span>
      <span class="ucl-bottom">UCL</span>
    </div>
    <div class="title-center-area">
      <div class="title">Jogo da Velha CNC</div>
    </div>
    <div id="modeButtons">
      <button onclick="setMode('machine')">Contra a Máquina</button>
      <button onclick="setMode('2players')">2 Jogadores</button>
      <button onclick="resetGame()">Jogar Novamente</button>
      <button onclick="resetScore()">Zerar Placar</button>
    </div>
  </div>
  <div class="score-container">
    <div class="score-card">
      <div class="score-card-title">PLACAR</div>
      <div class="score-bar">
        <span class="score-x" id="scoreX">0</span>
        <span class="score-o" id="scoreO">0</span>
      </div>
      <div id="starterDisplay">Próximo a jogar: Jogador 1</div>
    </div>
  </div>
  <div class="main">
    <table id="gameBoard">
      <tr><td onclick="makeMove(0)"></td><td onclick="makeMove(1)"></td><td onclick="makeMove(2)"></td></tr>
      <tr><td onclick="makeMove(3)"></td><td onclick="makeMove(4)"></td><td onclick="makeMove(5)"></td></tr>
      <tr><td onclick="makeMove(6)"></td><td onclick="makeMove(7)"></td><td onclick="makeMove(8)"></td></tr>
    </table>
    <div id="message"></div>
  </div>
  <div id="logBox">
    <div id="logTitle">Log de Comunicação</div>
    <div id="logContent"></div>
  </div>
  <div class="authors">
    Feito por: Daniel Oliveira Vettorazzi e Fábio Miranda Machado Júnior
  </div>
  <script>
    let board = Array(9).fill(''), currentPlayer = 'X', gameOver = false, mode = 'machine', score = {X:0,O:0}, starter = 'X', podeJogar = true, aguardandoBotao = false, delayProximo = false;

    function travarJogada() {
      podeJogar = false; aguardandoBotao = true;
      document.getElementById('message').innerText = "Aguardando confirmação do botão físico no tabuleiro...";
    }

    function aguardaDelayDestravamento() {
      podeJogar = false;
      aguardandoBotao = false;
      document.getElementById('message').innerText = "Aguardando próximo turno...";
      setTimeout(destravarJogada, 5000);
    }

    function destravarJogada() {
      podeJogar = true;
      aguardandoBotao = false;
      document.getElementById('message').innerText = "";
      if (mode === 'machine' && currentPlayer === 'O' && !gameOver) setTimeout(machineMove, 350);
    }

    function setMode(m) {
      mode = m; addLog(`Modo alterado para: ${m === 'machine' ? 'Contra a Máquina' : '2 Jogadores'}`);
      resetScore(); resetGame();
    }
    function updateScore() {
      document.getElementById('scoreX').innerText = score.X;
      document.getElementById('scoreO').innerText = score.O;
    }
    function makeMove(index) {
      if (!podeJogar || gameOver || board[index] !== '') return;
      board[index] = currentPlayer; updateBoard(); sendMove(index, currentPlayer);
      addLog(`${playerName(currentPlayer)} marcou na posição ${index + 1}`);
      if (checkWin(currentPlayer)) {
        document.getElementById('message').innerText = `${playerName(currentPlayer)} venceu!`;
        score[currentPlayer]++; updateScore(); gameOver = true;
        addLog(`${playerName(currentPlayer)} venceu a partida`); travarJogada(); return;
      }
      if (!board.includes('')) {
        document.getElementById('message').innerText = "Empate!"; gameOver = true;
        addLog("Partida terminou em empate"); travarJogada(); return;
      }
      travarJogada();
      currentPlayer = currentPlayer === 'X' ? 'O' : 'X';
    }
    function machineMove() {
      let move = findBestMove(); if (move !== -1) makeMove(move);
    }
    function findBestMove() {
      for (let i = 0; i < 9; i++) { if (board[i] === '') { board[i] = 'O'; if (checkWin('O')) { board[i] = ''; return i; } board[i] = ''; } }
      for (let i = 0; i < 9; i++) { if (board[i] === '') { board[i] = 'X'; if (checkWin('X')) { board[i] = ''; return i; } board[i] = ''; } }
      if (board[4] === '') return 4;
      const corners = [0,2,6,8]; for (let i of corners) if (board[i] === '') return i;
      for (let i = 0; i < 9; i++) if (board[i] === '') return i;
      return -1;
    }
    function checkWin(p) {
      const winPatterns = [[0,1,2],[3,4,5],[6,7,8],[0,3,6],[1,4,7],[2,5,8],[0,4,8],[2,4,6]];
      return winPatterns.some(pattern => pattern.every(index => board[index] === p));
    }
    function playerName(p) { return mode === 'machine' ? (p === 'X' ? 'Você' : 'Máquina') : (p === 'X' ? 'Jogador 1' : 'Jogador 2'); }
    function updateBoard() {
      const cells = document.querySelectorAll("td");
      board.forEach((val,i) => { cells[i].className = ''; if (val === 'X') cells[i].classList.add('player'); else if (val === 'O') cells[i].classList.add('machine'); cells[i].innerText = ''; });
    }
    function resetGame() {
      board = Array(9).fill(''); gameOver = false;
      starter = 'X'; // sempre começa com X
      currentPlayer = 'X';
      document.getElementById("message").innerText = ""; updateBoard();
      document.getElementById("starterDisplay").innerText = "Próximo a jogar: " + playerName(starter);
      fetch('/reset'); addLog("Novo jogo iniciado"); destravarJogada();
    }
    function resetScore() { score.X = 0; score.O = 0; updateScore(); addLog("Placar zerado"); }
    function sendMove(index, player) { fetch("/move?pos=" + index + "&player=" + player); }
    function addLog(msg) {
      const logContent = document.getElementById('logContent'), time = new Date().toLocaleTimeString('pt-BR', {hour:'2-digit',minute:'2-digit',second:'2-digit'});
      const el = document.createElement('div'); el.textContent = `[${time}] ${msg}`; logContent.appendChild(el); logContent.scrollTop = logContent.scrollHeight;
    }
    setInterval(() => {
      if (aguardandoBotao) {
        fetch('/podejogar')
          .then(res=>res.text())
          .then(val=>{
            if(val==='true') aguardaDelayDestravamento();
          });
      }
    }, 1000);
    updateScore(); addLog("Sistema pronto!");
  </script>
</body>
</html>


)rawliteral";

void setup() {
  Serial.begin(9600);      // Comunicação com Arduino Mega
  Serial2.begin(115200, SERIAL_8N1, RX2, TX2); // Comunicação com CNC

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }

  Serial.println("Conectado ao WiFi!");
  Serial.println(WiFi.localIP());

  delay(2000);
  Serial2.print("$X\n");  // Desbloqueia CNC

  server.on("/", []() {
    server.send(200, "text/html", htmlPage);
  });

  // Endpoint para jogada: só aceita se não estiver aguardando botão
  server.on("/move", []() {
    if (aguardandoBotao) {
      server.send(403, "text/plain", "Aguardando botão físico");
      return;
    }
    if (server.hasArg("pos") && server.hasArg("player")) {
      int pos = server.arg("pos").toInt();
      char player = server.arg("player")[0];

      if (pos >= 0 && pos < 9) {
        Serial2.print(positions[pos]);     // CNC move
        delay(2000);
        Serial2.print("G90 Z15\n");
        delay(1500);
        Serial2.print("G90 Z0\n");
        delay(500);
        Serial2.print("G90 X0 Y0\n");

        aguardandoBotao = true;            // Travar até apertar botão físico correto
        ultimaPosicao = pos;               // Salva a posição da última jogada
        ultimoPlayer = player;             // Salva o player da última jogada

        server.send(200, "text/plain", "Movimento enviado. Aguarde botão físico correto.");
      } else {
        server.send(400, "text/plain", "Posição inválida.");
      }
    } else {
      server.send(400, "text/plain", "Parâmetros 'pos' ou 'player' ausentes.");
    }
  });

  server.on("/reset", []() {
    Serial2.print("G90 X0 Y0 Z0\n");
    Serial.write('z');  // Desliga todos LEDs
    aguardandoBotao = false; // Libera para próxima jogada
    ultimaPosicao = -1;
    server.send(200, "text/plain", "Reiniciado!");
  });

  // Endpoint para polling do web: retorna "true" se pode jogar, "false" se travado
  server.on("/podejogar", []() {
    if (!aguardandoBotao)
      server.send(200, "text/plain", "true");
    else
      server.send(200, "text/plain", "false");
  });

  server.begin();
}

void loop() {
  server.handleClient();

  // Lê jogada do Mega (botões físicos)
  if (Serial.available()) {
    int input = Serial.parseInt();
    if (input == ultimaPosicao && input >= 0 && input < 9) {
      // Só libera se for o botão correto!
      char ledCode = (ultimoPlayer == 'X') ? ledCodes[ultimaPosicao][0] : ledCodes[ultimaPosicao][1];
      Serial.write(ledCode);  // Acende o LED da última jogada
      aguardandoBotao = false;
      ultimaPosicao = -1;
    }
    // Se for botão errado, simplesmente ignora
  }
}