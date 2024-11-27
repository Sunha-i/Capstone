from multi_env import multi_env

if __name__ == "__main__":
    ENVCOUNT = 2  # Number of environments
    multi_env = multi_env(ENVCOUNT)

    file_paths = [
        "result/final/26_out.txt",
        "result/final/26_out.txt"
    ]

    for id in range(ENVCOUNT):
        print(f"Result for ENV {id}")
        multi_env.reset(id)

    # for id, result in enumerate(results):
    #     print(f"Result for ENV {id}: {result}")
