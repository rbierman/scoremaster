class Player {
  final String name;
  final int number;
  final bool hasImage;

  Player({required this.name, required this.number, this.hasImage = false});

  factory Player.fromJson(Map<String, dynamic> json) {
    return Player(
      name: json['name'] as String,
      number: json['number'] as int,
      hasImage: json['hasImage'] as bool? ?? false,
    );
  }

  Map<String, dynamic> toJson() => {
    'name': name,
    'number': number,
    'hasImage': hasImage,
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
