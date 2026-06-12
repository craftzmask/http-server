import * as net from "net";
import type { HttpRequest } from "./types";
import { HttpResponse } from "./types";

const compressionSchemes = ["gzip"];

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

  // if the last line does not have semicolon,
  // then it should be request's body
  let body;
  if (!lines[lines.length - 1].includes(": ")) {
    body = lines[lines.length - 1];
  }

  return {
    method,
    path,
    version,
    headers,
    body,
  };
};

export const sendResponse = (
  socket: net.Socket,
  request: HttpRequest,
  content: string | Buffer = "",
  contentType: string = "text/plain",
): void => {
  const contentEncoding = request.headers["accept-encoding"];
  const response = HttpResponse.Builder.setContentType(
    contentType ? contentType : request.headers["content-type"],
  )
    .setContentEncoding(
      compressionSchemes.includes(contentEncoding) ? contentEncoding : "",
    )
    .setBody(content)
    .build();

  socket.write(response.toString());
};

export const sendOKResponse = (socket: net.Socket) => {
  socket.write(HttpResponse.Builder.build().toString());
};

export const sendNotFoundResponse = (socket: net.Socket) => {
  socket.write(
    HttpResponse.Builder.setStatusCode(404)
      .setStatusMessage("Not Found")
      .build()
      .toString(),
  );
};

export const sendCreateResponse = (socket: net.Socket) => {
  socket.write(
    HttpResponse.Builder.setStatusCode(201)
      .setStatusMessage("Created")
      .build()
      .toString(),
  );
};
