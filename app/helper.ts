import * as net from "net";
import type { HttpRequest } from "./types";
import { HttpResponse } from "./types";

export const parseRequest = (data: string | Buffer): HttpRequest => {
  const lines = data.toString().split("\r\n");
  const [method, path, version] = lines[0].split(" ");

  const headers: Record<string, string> = {};
  for (let i = 1; i < lines.length; i++) {
    if (lines[i].includes(": ")) {
      const [key, value] = lines[i].split(": ");
      headers[key.toLowerCase()] = value;
    }
  }

  return {
    method,
    path,
    version,
    headers,
  };
};

export const sendTextResponse = (socket: net.Socket, content: string): void => {
  const length = content.length;
  const response = HttpResponse.Builder.setHeaders({
    "content-type": "text/plain",
    "content-length": String(length),
  })
    .setBody(content)
    .build();
  socket.write(response.toString());
};
