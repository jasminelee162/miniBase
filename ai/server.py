import json
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse

try:
    from ai_helper import AIHelper
except Exception as e:
    AIHelper = None


class AIRequestHandler(BaseHTTPRequestHandler):
    helper = AIHelper() if AIHelper is not None else None

    def _send(self, code: int, payload: dict):
        body = json.dumps(payload).encode('utf-8')
        self.send_response(code)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        parsed = urlparse(self.path)
        if parsed.path != '/ai/chat':
            self._send(404, {"ok": False, "error": "not found"})
            return

        if AIRequestHandler.helper is None:
            self._send(500, {"ok": False, "error": "AIHelper not available"})
            return

        try:
            length = int(self.headers.get('Content-Length', '0'))
            raw = self.rfile.read(length) if length > 0 else b''
            data = json.loads(raw.decode('utf-8') or '{}')
        except Exception:
            self._send(400, {"ok": False, "error": "invalid json"})
            return

        prompt = (data.get('prompt') or '').strip()
        if not prompt:
            self._send(400, {"ok": False, "error": "empty prompt"})
            return

        try:
            # 调用你已有的 AI 助手，将自然语言转 SQL
            sql = AIRequestHandler.helper.nl_to_sql(prompt)
            self._send(200, {"ok": True, "reply": sql})
        except Exception as e:
            self._send(500, {"ok": False, "error": str(e)})


def run(host: str = '0.0.0.0', port: int = 9000):
    httpd = HTTPServer((host, port), AIRequestHandler)
    print(f"AI HTTP server running on http://{host}:{port}/ai/chat")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        httpd.server_close()


if __name__ == '__main__':
    h = '0.0.0.0'
    p = 9000
    if len(sys.argv) >= 2:
        # 支持: python server.py 127.0.0.1:9001 或 python server.py 9001
        arg = sys.argv[1]
        if ':' in arg:
            h, ps = arg.split(':', 1)
            try:
                p = int(ps)
            except Exception:
                pass
        else:
            try:
                p = int(arg)
            except Exception:
                pass
    run(h, p)


