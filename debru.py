def b(number):
    return format(number, '08b')


def kadem(n, bit):
    global range_lower, range_upper
    nodes = list(range(1 << bit))
    for node_move in nodes:
        for j in range(bit):
            if (node_move >> j) & 0x1 == 1:
                range_lower = ((node_move >> (j + 1)) << (j + 1))
                range_upper = ((node_move >> (j + 1)) << (j + 1)) + (1 << j)
            else:
                range_lower = ((node_move >> (j + 1)) << (j + 1)) + (1 << j)
                range_upper = ((node_move >> (j + 1)) << (j + 1)) + (1 << (j + 1))
        print(f"node_move = {b(node_move)}, range_lower = {b(range_lower)}, range_upper = {b(range_upper)}")


if __name__ == '__main__':
    kadem(4, 4)
