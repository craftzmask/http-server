import * as net from "net";
import { readFileSync, existsSync } from "fs";
import { parseRequest } from "./helper";
import { sendTextResponse } from "./helper";
import { HttpResponse } from "./types";

// const filename = process.argv[3];
// const content = readFileSync(filename + "foo");
// console.log("content", content);

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
    } else if (path.startsWith("/files")) {
      const dir = process.argv[3];
      const filename = path.substring("/files/".length);
      const fullPath = `${dir}${filename}`;
      if (!existsSync(fullPath)) {
        socket.write("HTTP/1.1 404 Not Found\r\n\r\n");
        return;
      }

      const content = readFileSync(fullPath);
      const response = HttpResponse.Builder.setHeaders({
        "content-type": "application/octet-stream",
        "content-length": content.length,
      })
        .setBody(content)
        .build();
      socket.write(response.toString());
    } else {
      socket.write("HTTP/1.1 404 Not Found\r\n\r\n");
    }
  });

  socket.on("close", () => socket.end());
});

server.listen(4221, "localhost");
