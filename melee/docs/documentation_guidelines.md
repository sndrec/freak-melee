# Writing reference documentation

Explain what each feature does, when to use it, and what its inputs and results
mean. Use plain words and direct sentences. Start with the behavior a script
author or Workshop user can observe.

- Describe callbacks by when they run and what each return value does.
- Give values their types, units, ranges and meaningful defaults.
- Explain unfamiliar terms when first introducing them.
- Use small examples with real, supported syntax.
- Put requirements and limitations beside the behavior they affect. Explain
  their practical consequence.
- Keep implementation history and test results in progress or checkpoint
  documents. Link them from the reference when they help explain a feature.

The scripting API uses generic functions and named fighter variables and
parameters. Document each function in terms that apply to any custom fighter.
Use stock moves as examples of those functions.

Use Godot's [Node3D](https://docs.godotengine.org/en/stable/classes/class_node3d.html)
and [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html)
references as examples of concise descriptions, typed member lists and
explanations linked to related features.

## API implementation rule

Expose reusable engine operations. Store each fighter's charges, cooldowns and
move settings in named `state` variables and `parameter` declarations.
Do not add API properties named after a stock fighter, special move or old
fighter-union field, such as `charge.cyclone`. Implement that charge as a normal
fighter variable, and document the generic storage feature with a move example.
