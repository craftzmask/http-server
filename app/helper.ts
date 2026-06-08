import type { HttpRequest } from "./types";

export const parseRequest = (data: string | Buffer): HttpRequest => {
  const lines = data.toString().split("\r\n");
  const [method, path, version] = lines[0].split(" ");
  return {
    method,
    path,
    version,
  };
};
