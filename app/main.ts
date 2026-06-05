import * as net from "net";

interface HttpRequest {
  method: string;
  path: string;
  version: string;
}

const parseRequest = (data: string | Buffer): HttpRequest => {
  const lines = data.toString().split("\r\n");
  const [method, path, version] = lines[0].split(" ");
  return {
    method,
    path,
    version,
  };
};

const server = net.createServer((socket) => {
  socket.on("data", (data) => {
    const request = parseRequest(data);

    if (request.path === "/") {
      socket.write("HTTP/1.1 200 OK\r\n\r\n");
    } else {
      socket.write("HTTP/1.1 404 Not Found\r\n\r\n");
    }
  });

  socket.on("close", () => {
    socket.end();
  });
});

server.listen(4221, "localhost");
