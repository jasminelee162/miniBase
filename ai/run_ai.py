from ai_helper import AIHelper

if __name__ == "__main__":
    ai = AIHelper()
    print("=== AI SQL 助手 ===")
    while True:
        user_input = input("\n请输入自然语言描述 (输入 exit 退出): ")
        if user_input.lower() in ["exit", "quit"]:
            break
        sql = ai.nl_to_sql(user_input)
        print("生成的 SQL:\n", sql)
