import os
import json

class OmpModifierReference:
    def __init__(self, name, modifying):
        self.name = name
        self.modifying = modifying

class OmpModifier:
    def __init__(self, name, type, items, modifying, properties):
        self.name = name
        self.type = type
        self.items = items
        self.modifying = modifying
        self.properties = properties

class OmpClauseGroup:
    def __init__(self, name, clauses, properties):
        self.name = name
        self.clauses = clauses
        self.properties = properties

class OmpClause:
    def __init__(self, name, arguments, properties, modifiers):
        self.name = name
        self.arguments = arguments
        self.properties = properties
        self.modifiers = modifiers

class OmpDirective:
    def __init__(self, name, association, category, clauses, properties,
                 clause_groups, arguments, association_properties):
        self.name = name
        self.association = association
        self.category = category
        self.clauses = clauses
        self.properties = properties
        self.clause_groups = clause_groups
        self.arguments = arguments
        self.association_properties = association_properties

class OmpStore:
    def __init__(self):
        self.clauses = {}
        self.clause_groups = {}
        self.directives = {}
        self.modifiers = {}

    def add_modifier(self, json):
        body = json["modifier type"]
        mod_name = json["$name"]
        modifier = OmpModifier(
            name=mod_name,
            type=body["$type"],
            items=body.get("items"),
            modifying=json.get("modifying"),
            properties=json.get("properties", [])
        )
        self.modifiers[mod_name] = modifier

    def add_clause_group(self, json):
        group_name = json["$name"]
        group = OmpClauseGroup(
            name=group_name,
            clauses=json.get("clauses", []),
            properties=json.get("properties", [])
        )
        self.clause_groups[group_name] = group

    def add_clause(self, json):
        clause_name = json["$name"]
        modifiers = []
        for mod in json.get("modifiers", []):
            mod_name = mod["$name"]
            if mod_name in self.modifiers:
                modifiers.append(OmpModifierReference(
                    mod_name, mod["modifying"]))
            else:
                modifiers.append(OmpModifier(
                    name=mod_name,
                    type=mod["$type"],
                    items=mod.get("items"),
                    modifying=mod.get("modifying"),
                    properties=mod.get("properties", [])
                ))
        self.clauses[clause_name] = OmpClause(
            name=clause_name,
            arguments=json.get("arguments", []),
            properties=json.get("properties", []),
            modifiers=modifiers
        )

    def add_directive(self, json):
        dir_name = json["$name"]
        self.directives[dir_name] = OmpDirective(
            name=dir_name,
            association=json["association"],
            category=json["category"],
            clauses=json.get("clauses", []),
            clause_groups=json.get("clause-groups", []),
            properties=json.get("properties", []),
            arguments=json.get("arguments", []),
            association_properties=json.get("association properties", [])
        )

def read_jsons(path):
    jsons = []
    for json_file in os.listdir(path):
        full_path = os.path.join(path, json_file)
        print(full_path)
        if os.path.isfile(full_path) and json_file.endswith(".json"):
            with open(full_path) as f:
                jsons.append(json.loads(f.read()))
    return jsons

def read_dir(spec_path):
    json_dir = os.path.join(spec_path, "json")
    store = OmpStore()
    
    for json in read_jsons(os.path.join(json_dir, "modifier")):
        store.add_modifier(json)

    for json in read_jsons(os.path.join(json_dir, "clause")):
        store.add_clause(json)    

    for json in read_jsons(os.path.join(json_dir, "clause-group")):
        store.add_clause_group(json)

    for json in read_jsons(os.path.join(json_dir, "directive")):
        store.add_directive(json)

    return store
