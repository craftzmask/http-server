import * as net from "net";
import { HttpResponse } from "./types";
import { parseRequest } from "./helper";

const server = net.createServer((socket) => {
  socket.on("data", (data) => {
    const request = parseRequest(data);

    if (request.path === "/") {
      socket.write("HTTP/1.1 200 OK\r\n\r\n");
    } else if (request.path.startsWith("/echo")) {
      const content = request.path.substring("/echo/".length);
      const length = content.length;
      const response = HttpResponse.Builder.setHeaders({
        "content-type": "text/plain",
        "content-length": String(length),
      })
        .setBody(content)

        .build();
      socket.write(response.toString());
    } else {
      socket.write("HTTP/1.1 404 Not Found\r\n\r\n");
    }
  });

  socket.on("close", () => {
    socket.end();
  });
});

server.listen(4221, "localhost");
