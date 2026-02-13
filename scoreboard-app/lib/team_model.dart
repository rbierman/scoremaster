class Player {
  final String name;
  final int number;

  Player({required this.name, required this.number});

  factory Player.fromJson(Map<String, dynamic> json) {
    return Player(
      name: json['name'] as String,
      number: json['number'] as int,
    );
  }

  Map<String, dynamic> toJson() => {
    'name': name,
    'number': number,
  };
}

class Team {
  final String name;
  final List<Player> players;

  Team({required this.name, required this.players});

  factory Team.fromJson(Map<String, dynamic> json) {
    return Team(
      name: json['name'] as String,
      players: (json['players'] as List)
          .map((e) => Player.fromJson(e as Map<String, dynamic>))
          .toList(),
    );
  }

  Map<String, dynamic> toJson() => {
    'name': name,
    'players': players.map((e) => e.toJson()).toList(),
  };
}
