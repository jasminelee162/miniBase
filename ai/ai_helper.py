import os
from dotenv import load_dotenv
from openai import OpenAI

# 加载 .env 文件中的环境变量
load_dotenv()

class AIHelper:
    def __init__(self):
        self.client = OpenAI(
            api_key=os.getenv("DASHSCOPE_API_KEY"),
            base_url="https://dashscope.aliyuncs.com/compatible-mode/v1",
        )

    def nl_to_sql(self, user_input: str) -> str:
        """
        把自然语言描述转换为 SQL 语句
        """
        try:
            completion = self.client.chat.completions.create(
                model="qwen-plus",
                messages=[
                    {
                        "role": "system",
                        "content": (
                            "你是一个 SQL 助手，把用户的自然语言需求转成标准 SQL 语句。"
                            "只输出 SQL，不要解释，SQL语句都使用英文，若用户发送其他对话则用中文回答。"
                        ),
                    },
                    {"role": "user", "content": user_input},
                ],
            )
            return completion.choices[0].message.content.strip()
        except Exception as e:
            return f"-- AI 调用出错: {e}"
