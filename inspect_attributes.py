import json

with open('bosses_complete.json', 'r') as f:
    data = json.load(f)

attributes = set()
for monster in data:
    if 'attributes' in monster:
        for attr in monster['attributes']:
            attributes.add(attr)

print(sorted(list(attributes)))
