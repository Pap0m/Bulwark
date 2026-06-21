class Bulwark
  def self.New
    @instance ||= new
  end

  def initialize
    @router = Router.new
  end

  def get(uri, handler)
    @router.add_route("GET", uri, handler)
  end

  def dispatch(req, res)
    handler = @router.find_route(req.method, req.uri)

    if handler
      Object.send(handler, req, res)
    end

    res
  end
end

class Router
  def initialize
    @routes = {}
  end

  def add_route(method, uri, handler)
    @routes["#{method}:#{uri}"] = handler
  end

  def find_route(method, uri)
    @routes["#{method}:#{uri}"]
  end
end

class Request
  attr_reader :method, :uri, :headers, :body

  def initialize(method, uri, headers, body)
    @method = method
    @uri = uri
    @headers = headers
    @body = body
  end
end

class Response
  attr_reader :status, :cheaders, :body

  def initialize
    @status = 200
    @cheaders = ""
    @body = ""
  end

  def reply(status, headers, body)
    @status = status
    @cheaders = headers.map { |k, v| "#{k}: #{v}\r\n" }.join
    puts @cheaders
    @body = body
  end
end

def user_handler(req, res)
  puts "--> Handling #{req.method} #{req.uri}"
  headers = {
    "Content-Type" => "application/json",
    "Set-Cookie" => "name=value"
  }

  json = '{"id": 1, "username": "apidog", "password": "supersecret"}'

  res.reply(200, headers, json)
end

# TODO: Use DSL metaprogramming
router = Bulwark.New
router.get("/users", :user_handler)
