import requests

GREEN = "\033[0;32m"
LIGHT_GREEN = "\033[1;32m"
BOLD = "\033[1m"
CYAN = "\033[0;36m"
END = "\033[0m"

API_URL = "https://api.openai.com/v1/chat/completions"
API_TOKEN = "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"

headers = {
  "Content-Type": "application/json",
  "Authorization": f"Bearer {API_TOKEN}"
}

data = {
  "model": "gpt-3.5-turbo-0301",
  "temperature": 0.95,
  "max_tokens": 1000,
  "user": "comp304",
  "messages": [
    {
      "role": "system",
      "content": "You are a Linux Terminal called 'Michelle'. User will type commands and you will reply with what the terminal should show. Reply with the terminal output ONLY inside one unique code block, and nothing else. DO NOT write explanations. DO NOT type commands unless the user instructs you to do so."
    },
    {
      "role": "user",
      "content": 'Michelle, you are my personal Linux terminal. I will type commands and you will reply with what the terminal should show. I want you to only reply with the terminal output, and nothing else. DO NOT write explanations. DO NOT type commands unless I instruct you to do so. Write "(null)" when there is no output. DO NOT write hints or explanations in parentheses or brackets. When I need to tell you something in English I will do so by putting text inside curly brackets {like this}.'
    }
  ]
}

def add_message(content):
  data["messages"].append({"role": "user", "content": content})

def add_response(content):
  data["messages"].append({"role": "assistant", "content": content})

def request(message):
  add_message(message)
  res = requests.post(url=API_URL, json=data, headers=headers)
  resdict = res.json()
  resp = resdict["choices"][0]["message"]["content"]
  add_response(resp)
  return resp

def prompt():
  while True:
    user_message = input(f"{CYAN}Michelle{END} $ ")
    if (user_message == "exit"):
      print("Bye! Until next time!")
      exit(0)
    try:
      assistant_response = request(user_message)
      print(assistant_response)
    except Exception as _:
      add_response("I don't understand. Can you repeat?")
      print("Michelle doesn't understand the input. Try again?")
      continue

if __name__ == "__main__":
  print(f"Hello, I'm {BOLD}{GREEN}\"My-Interactive-Shell\"{END} a.k.a {BOLD}{LIGHT_GREEN}Michelle{END}.")
  print("I'm here to execute your commands. Type 'exit' to exit the shell.")
  prompt()