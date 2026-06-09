import * as net from "net";
import { parseRequest } from "./helper";
import { sendTextResponse } from "./helper";

const server = net.createServer((socket) => {
  socket.on("data", (data) => {
    const request = parseRequest(data);

    const path = request.path;
    if (path === "/") {
      socket.write("HTTP/1.1 200 OK\r\n\r\n");
    } else if (path.startsWith("/echo")) {
      const content = path.substring("/echo/".length);
      sendTextResponse(socket, content);
    } else if (path.startsWith("/user-agent")) {
      const content = request.headers["user-agent"];
      sendTextResponse(socket, content);
    } else {
      socket.write("HTTP/1.1 404 Not Found\r\n\r\n");
    }
  });

  socket.on("close", () => socket.end());
});

server.listen(4221, "localhost");
