export interface HttpRequest {
  method: string;
  path: string;
  version: string;
  headers: Record<string, string>;
  body?: string;
}

export class HttpResponse {
  constructor(
    public version: string,
    public statusCode: number,
    public statusMessage: string,
    public headers: Record<string, string | number>,
    public body?: string | Buffer,
  ) {}

  static get Builder() {
    return new ResponseBuilder();
  }

  toString(): string {
    const lines = [`${this.version} ${this.statusCode} ${this.statusMessage}`];
    Object.entries(this.headers).forEach((header) =>
      lines.push(`${header[0]}: ${header[1]}`),
    );

    // Mark to end of headers
    lines.push("");

    if (this.body) {
      lines.push(`${this.body}`);
    } else {
      lines.push("");
    }

    return lines.join("\r\n");
  }

  toBuffer(): Buffer {
    const lines = [`${this.version} ${this.statusCode} ${this.statusMessage}`];
    Object.entries(this.headers).forEach((header) =>
      lines.push(`${header[0]}: ${header[1]}`),
    );

    if (this.body) {
      lines.push("");
      lines.push("");
      console.log("lines", lines);
      console.log(lines.join("\r\n"), this.body);
      console.log(this.body);

      return Buffer.concat([
        Buffer.from(lines.join("\r\n")),
        Buffer.from(this.body),
      ]);
    }

    return Buffer.from(lines.join("\r\n"));
  }
}

class ResponseBuilder {
  private version = "HTTP/1.1";
  private statusCode = 200;
  private statusMessage = "OK";
  private headers: Record<string, string | number> = {};
  private body?: string | Buffer;

  setVersion(version: string): this {
    this.version = version;
    return this;
  }

  setStatusCode(statuCode: number): this {
    this.statusCode = statuCode;
    return this;
  }

  setStatusMessage(statusMessage: string): this {
    this.statusMessage = statusMessage;
    return this;
  }

  setContentType(contentType: string): this {
    this.setHeader("content-type", contentType);
    return this;
  }

  setContentEncoding(contentEncoding: string): this {
    if (contentEncoding || contentEncoding.length > 0) {
      this.setHeader("content-encoding", contentEncoding);
    }
    return this;
  }

  setHeaders(headers: Record<string, string | number>): this {
    this.headers = headers;
    return this;
  }

  setHeader(key: string, value: string | number): this {
    this.headers[key] = value;
    return this;
  }

  setBody(bodyText: string | Buffer | Uint8Array): this {
    if (bodyText || bodyText.length > 0) {
      this.body = Buffer.from(bodyText);
      this.setHeader("content-length", this.body.length);
    }

    return this;
  }

  build(): HttpResponse {
    return new HttpResponse(
      this.version,
      this.statusCode,
      this.statusMessage,
      this.headers,
      this.body,
    );
  }
}
